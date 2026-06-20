#!/usr/bin/env sh

set -eu

root_dir=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
script_path="$root_dir/.codex/hooks/run-extract-i18n-tests.sh"

if [ "${CONDA_DEFAULT_ENV:-}" != "ia" ]; then
    if [ "${IA_TEST_CONDA_REEXEC:-}" = "1" ]; then
        echo "extract-i18n test hook: expected conda env 'ia', got '${CONDA_DEFAULT_ENV:-unset}'." >&2
        exit 1
    fi

    conda_exe=${CONDA_EXE:-}
    if [ -z "$conda_exe" ] || [ ! -x "$conda_exe" ]; then
        conda_exe=$(command -v conda 2>/dev/null || true)
    fi
    if [ -z "$conda_exe" ]; then
        echo "extract-i18n test hook: conda is required; cannot enter env 'ia'." >&2
        exit 1
    fi

    echo "extract-i18n test hook: entering conda env 'ia'."
    exec env IA_TEST_CONDA_REEXEC=1 "$conda_exe" run -n ia --no-capture-output "$script_path"
fi

cd "$root_dir"
changed_files=$(
    {
        git diff --name-only HEAD -- CMakeLists.txt include src test installed_files/data/locale 2>/dev/null || true
        git ls-files --others --exclude-standard -- CMakeLists.txt include src test installed_files/data/locale 2>/dev/null || true
    } | sort -u
)

if [ -z "$changed_files" ] && [ "${IA_TEST_FORCE:-}" != "1" ]; then
    echo "extract-i18n test hook: no relevant source, locale, or test changes; skipping."
    exit 0
fi

if [ -n "$changed_files" ]; then
    echo "extract-i18n test hook: relevant changes detected:"
    printf '%s\n' "$changed_files"
fi

build_dir=build-linux-tests
jobs=${IA_TEST_JOBS:-$(nproc)}

cmake -S . -B "$build_dir" \
    -DCMAKE_C_COMPILER="${CC:-/usr/bin/cc}" \
    -DCMAKE_CXX_COMPILER="${CXX:-/usr/bin/c++}"
cmake --build "$build_dir" --target ia-test -- -j"$jobs"

cd "$build_dir"

if [ -n "${IA_TEST_FILTER:-}" ]; then
    exec ./ia-test -D 3 --abort "$IA_TEST_FILTER"
fi

exec ./ia-test -D 3 --abort
