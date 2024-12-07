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

chkeol_date() {
    local PROJECT="$1"
    local CYCLE="$2"
    local ENDOFLIFE_AFTER="$3"
    local ENDOFLIFE_TRESHOLD="${4:--3 months}"

    if ! [[ "$ENDOFLIFE_AFTER" =~ ^([1-9][0-9]{3})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])$ ]]; then
        echo "Invalid end of life date: ${ENDOFLIFE_AFTER@Q} doesn't match 'YYYY-MM-DD' format" >&2
        return 1
    fi

    local NOTIFY_AFTER="$(date -u -d "${ENDOFLIFE_AFTER}T00:00:00Z $ENDOFLIFE_TRESHOLD" +'%F')"
    local TODAY="$(date -u -d '00:00:00Z' +'%F')"

    if ! [[ "$TODAY" > "$NOTIFY_AFTER" ]]; then
        echo "$PROJECT $CYCLE is supported until $ENDOFLIFE_AFTER"
        return 0
    elif ! [[ "$TODAY" > "$ENDOFLIFE_AFTER" ]]; then
        echo "$PROJECT $CYCLE is supported until $ENDOFLIFE_AFTER, but the notification treshold was reached"
        return 1
    else
        echo "$PROJECT $CYCLE reached its end of life on $ENDOFLIFE_AFTER"
        return 1
    fi
}

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
