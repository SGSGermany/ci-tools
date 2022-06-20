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
    local TYPE= INFO=
    local SRC="$2"
    local DIR="$3"

    [ "$1" != "--local" ] || { TYPE="local"; SRC_INFO="./base-conf"; }
    [ "$1" != "--upstream" ] || { TYPE="upstream"; SRC_INFO="…"; }
    [ -n "$TYPE" ] || return 1
    shift 3

    while [ $# -ge 2 ]; do
        if [ ! -d "$DIR/raw/$TYPE/$(dirname "$2")" ]; then
            echo + "mkdir -p /tmp/…/{raw,clean}/$TYPE/$(dirname "$2")" >&2
            mkdir -p "$DIR/raw/$TYPE/$(dirname "$2")" "$DIR/clean/$TYPE/$(dirname "$2")"
        fi

        echo + "cp $SRC_INFO/$1 /tmp/…/raw/$TYPE/$2" >&2
        cp "$SRC/$1" "$DIR/raw/$TYPE/$2"

        echo + "chkconf_clean /tmp/…/raw/$TYPE/$2 /tmp/…/clean/$TYPE/$2" >&2
        chkconf_clean "$DIR/raw/$TYPE/$2" "$DIR/clean/$TYPE/$2"
        shift 2
    done
}

chkconf_clean() {
    sed -e '/^\s*$/d' "$1" > "$2"
}

chkconf_diff() {
    local DIR="$1"

    echo + "diff -q -r /tmp/…/clean/local/ /tmp/…/clean/upstream/" >&2
    if ! diff -q -r "$DIR/clean/local/" "$DIR/clean/upstream/" > /dev/null; then
        ( cd "$DIR/raw" ; diff -u -r ./local/ ./upstream/ )
        return 1
    fi
}
