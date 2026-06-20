#!/usr/bin/env sh

set -eu

input=$(cat || true)

is_git_add=$(
    printf '%s\n' "$input" |
        python3 -c '
import json
import re
import sys

try:
    payload = json.load(sys.stdin)
except Exception:
    sys.exit(0)

commands = []

def collect(value):
    if isinstance(value, dict):
        for key, item in value.items():
            if key in ("cmd", "command") and isinstance(item, str):
                commands.append(item)
            collect(item)
    elif isinstance(value, list):
        for item in value:
            collect(item)

collect(payload)
pattern = re.compile(r"(^|[;&|]\s*)git\s+add(\s|$)")
if any(pattern.search(command) for command in commands):
    print("1")
'
)

if [ "$is_git_add" != "1" ]; then
    exit 0
fi

root_dir=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
cd "$root_dir"

changed_files=$(
    {
        git diff --name-only HEAD -- CMakeLists.txt include src test installed_files/data/locale 2>/dev/null || true
        git ls-files --others --exclude-standard -- CMakeLists.txt include src test installed_files/data/locale 2>/dev/null || true
    } | sort -u
)

if [ -z "$changed_files" ]; then
    exit 0
fi

changed_hash=$(
    printf '%s\n' "$changed_files" |
        while IFS= read -r path; do
            if [ -e "$path" ]; then
                printf '%s\t%s\n' "$path" "$(git hash-object -- "$path")"
            else
                printf '%s\t%s\n' "$path" "deleted"
            fi
        done | git hash-object --stdin
)

stamp_path="$root_dir/build-linux-tests/extract-i18n-tests.ok"

if [ -f "$stamp_path" ] && [ "$(cat "$stamp_path")" = "$changed_hash" ]; then
    exit 0
fi

{
    echo "extract-i18n pre-add hook: i18n tests have not passed for the current relevant changes."
    echo "Run this command before staging extraction changes:"
    echo "  .codex/hooks/run-extract-i18n-tests.sh"
    echo
    echo "Relevant changed files:"
    printf '  %s\n' $changed_files
} >&2

exit 2
