# ci-tools -- helper/chkconf.sh.inc
# Helper functions to check config files for changed default settings.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

chkconf_prepare() {
    local TYPE=
    local SRC="$2"
    local SRC_INFO="$3"
    local DIR="$4"
    local DIR_INFO="$5"

    [ "$1" != "--local" ] || TYPE="local"
    [ "$1" != "--upstream" ] || TYPE="upstream"
    [ -n "$TYPE" ] || return 1
    shift 5

    while [ $# -ge 2 ]; do
        if [ ! -d "$DIR/raw/$TYPE/$(dirname "$2")" ]; then
            echo + "mkdir -p $(quote "$DIR_INFO/{raw,clean}/$TYPE/$(dirname "$2")")" >&2
            mkdir -p "$DIR/raw/$TYPE/$(dirname "$2")" "$DIR/clean/$TYPE/$(dirname "$2")"
        fi

        echo + "cp $(quote "$SRC_INFO/$1") $(quote "$DIR_INFO/raw/$TYPE/$2")" >&2
        cp "$SRC/$1" "$DIR/raw/$TYPE/$2"

        echo + "chkconf_clean $(quote "$DIR_INFO/raw/$TYPE/$2") $(quote "$DIR_INFO/clean/$TYPE/$2")" >&2
        chkconf_clean "$DIR/raw/$TYPE/$2" "$DIR/clean/$TYPE/$2"
        shift 2
    done
}

chkconf_clean() {
    sed -e '/^\s*$/d' "$1" > "$2"
}

chkconf_diff() {
    local DIR="$1"
    local DIR_INFO="${2:-$DIR}"

    echo + "diff -q -r $(quote "$DIR_INFO/clean/local/") $(quote "$DIR_INFO/clean/upstream/")" >&2
    if ! diff -q -r "$DIR/clean/local/" "$DIR/clean/upstream/" > /dev/null; then
        ( cd "$DIR/raw" ; diff -u -r ./local/ ./upstream/ )
        return 1
    fi
}
