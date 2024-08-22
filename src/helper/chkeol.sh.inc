# ci-tools -- helper/chkeol.sh.inc
# Helper functions to check whether a version is still actively supported.
#
# Copyright (c) 2024  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

chkeol_versions() {
    local PATTERN="${1:-^v?([0-9]+\.[0-9]+(\.[0-9]+(\.[0-9]+)?(-[a-zA-Z0-9.-]+)?(\+[a-zA-Z0-9.-]+)?)?)$}"

    local VERSION
    local VERSIONS=()

    echo + "while read -r VERSION; do [[ \"\$VERSION\" =~ $PATTERN ]]; done" >&2
    while read -r VERSION; do
        if [[ "$VERSION" =~ $PATTERN ]]; then
            VERSIONS+=( "${BASH_REMATCH[1]:-$VERSION}" )
        fi
    done

    if [ ${#VERSIONS[@]} -eq 0 ]; then
        echo "Invalid versions list: No version matches the given version pattern" >&2
        return 1
    fi

    echo + "sort_semver" >&2
    printf '%s\n' "${VERSIONS[@]}" | sort_semver
}

chkeol_latest_minor() {
    local PROJECT="$1"
    local CYCLE="$2"

    echo + "LATEST_CYCLE=\"\$(sed -ne '1 s/^\([0-9][0-9]*\.[0-9][0-9]*\)\(\...*\)\?$/\1/p')\"" >&2
    local LATEST_CYCLE="$(sed -ne '1 s/^\([0-9][0-9]*\.[0-9][0-9]*\)\(\...*\)\?$/\1/p')"

    if [ -z "$LATEST_CYCLE" ]; then
        echo "Invalid release cycle: Unable to determine latest minor version as release cycle" >&2
        return 1
    fi

    echo + "[[ $(quote "$CYCLE") =~ ^[0-9]+\.[0-9]+$ ]]" >&2
    if [[ ! "$CYCLE" =~ ^[0-9]+\.[0-9]+$ ]]; then
        echo "Invalid release cycle: Invalid current release cycle given: $CYCLE" >&2
        return 1
    fi

    if [ "$CYCLE" == "$LATEST_CYCLE" ]; then
        echo "$PROJECT $CYCLE is the latest release cycle and thus supported"
        return 0
    else
        echo "$PROJECT $CYCLE is not supported, the latest release cycle is $PROJECT $LATEST_CYCLE"
        return 1
    fi
}

