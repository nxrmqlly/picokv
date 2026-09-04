#!/usr/bin/env bash

set -euo pipefail

HEADER='// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026-present Ritam Das'

while IFS= read -r -d '' file; do
    if grep -qF 'SPDX-License-Identifier: GPL-3.0-or-later' "$file"; then
        echo "Skipping: $file"
        continue
    fi

    tmp=$(mktemp)

    {
        printf '%s\n\n' "$HEADER"
        cat "$file"
    } > "$tmp"

    mv "$tmp" "$file"

    echo "Licensed: $file"
done < <(
    find . -type f \( -name '*.c' -o -name '*.h' \) -print0
)
