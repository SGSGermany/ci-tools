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
