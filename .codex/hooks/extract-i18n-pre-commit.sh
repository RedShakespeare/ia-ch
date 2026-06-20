#!/usr/bin/env sh

set -eu

input=$(cat || true)

if ! printf '%s\n' "$input" | grep -Eq '"(command|cmd)"[[:space:]]*:[[:space:]]*"([^"]*[[:space:]])?git[[:space:]]+add([[:space:]]|")'; then
    exit 0
fi

root_dir=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
exec "$root_dir/.codex/hooks/run-extract-i18n-tests.sh"
