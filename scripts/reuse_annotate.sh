#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 lj970926
#
# SPDX-License-Identifier: MIT

set -euo pipefail

if command -v reuse >/dev/null 2>&1; then
    reuse_cmd=(reuse)
elif [ -x "${HOME}/.venv/bin/python3" ]; then
    reuse_cmd=("${HOME}/.venv/bin/python3" -m reuse)
else
    echo "reuse not found; activate ~/.venv or install reuse." >&2
    exit 1
fi

status=0

for path in "$@"; do
    [ -f "$path" ] || continue

    case "$path" in
        CMakeLists.txt|*/CMakeLists.txt|*.cmake)
            style_args=(--style python)
            ;;
        *.h|*.hh|*.hpp)
            style_args=(--style cpp --multi-line)
            ;;
        *.cc|*.cpp|*.cxx)
            style_args=(--style cppsingle --single-line)
            ;;
        *)
            continue
            ;;
    esac

    if ! "${reuse_cmd[@]}" annotate \
        --copyright "lj970926" \
        --year 2026 \
        --license MIT \
        --skip-existing \
        "${style_args[@]}" \
        "$path"; then
        status=1
    fi
done

exit "$status"
