# ci-tools -- helper/common.sh.inc
# Common helper functions for @SGSGermany's build scripts.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

[ -x "$(which rsync 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: rsync" >&2; exit 1; }

quote() {
    local QUOTED=""
    for ARG in "$@"; do
        [ "$(printf '%q' "$ARG")" == "$ARG" ] \
            && QUOTED+=" $ARG" \
            || QUOTED+=" ${ARG@Q}"
    done

    echo "${QUOTED:1}"
}

cmd() {
    echo + "$(quote "$@")" >&2
    "$@"
    return $?
}

trunc() {
    for FILE in "$@"; do
        if [ -f "$FILE" ]; then
            : > "$FILE"
        elif [ -d "$FILE" ]; then
            find "$FILE" -mindepth 1 -delete
        fi
    done
}

sponge() {
    local FILE="$1"
    local CONTENT="$(cat)"
    [ -n "$CONTENT" ] && printf '%s\n' "$CONTENT" > "$FILE" || : > "$FILE"
}

sort_semver() {
    sed -e 's/^\([^+~-]*\)\([^+~]*\)\(.*\)$/\1\t\2_\t\3/' | sort -V -r | sed -e 's/_\?\t//g'
}

version_greater() {
    [ "$(printf '%s\n' "$@" | sort_semver | tail -n 1)" != "$1" ]
}

_curl() {
    local RETURN_CODE=0
    local RESPONSE="$(curl -sSL -i "$@" || { RETURN_CODE=$?; true; })"

    local HEADERS="$(sed -ne '1,/^\r$/{s/\r$//p}' <<< "$RESPONSE")"
    local BODY="$(sed -e '1,/^\r$/d' <<< "$RESPONSE")"

    local STATUS_CODE="$(sed -ne '1{s#^HTTP/[0-9.]* \([0-9]*\)\( .*\)\?$#\1#p}' <<< "$HEADERS")"
    if [ -z "$STATUS_CODE" ] || (( $STATUS_CODE < 100 )) || (( $STATUS_CODE >= 300 )); then
        [ $RETURN_CODE -ne 0 ] || RETURN_CODE=22

        local STATUS_STRING="$(sed -ne '1{s#^HTTP/[0-9.]* \(.*\)$#\1#p}' <<< "$HEADERS")"
        echo "curl: ($RETURN_CODE) The requested URL returned error: $STATUS_STRING" >&2
        printf '%s\n' "$BODY" >&2
    fi

    printf '%s\n\n%s\n' "$HEADERS" "$BODY"
    return $RETURN_CODE
}
