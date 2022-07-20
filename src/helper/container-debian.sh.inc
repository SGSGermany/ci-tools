# ci-tools -- helper/container-debian.sh.inc
# Helper functions for @SGSGermany's containers based on Debian GNU/Linux.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

cleanup() {
    local CONTAINER="$1"
    local MOUNT="$(buildah mount "$CONTAINER")"

    cmd buildah run "$CONTAINER" -- \
        apt-get clean

    echo + "trunc …/var/lib/apt/lists …/var/log/{lastlog,faillog,wtmp,btmp}"
    trunc \
        "$MOUNT/var/lib/apt/lists" \
        "$MOUNT/var/log/btmp" \
        "$MOUNT/var/log/faillog" \
        "$MOUNT/var/log/lastlog" \
        "$MOUNT/var/log/wtmp"

    echo + "rm -f …/etc/{passwd,shadow}- …/etc/{group,gshadow}- …/etc/resolv.conf" \
        "…/var/log/alternatives.log …/var/log/apt/{history.log,term.log} …/var/log/dpkg.log"
    rm -f \
        "$MOUNT/etc/passwd-" \
        "$MOUNT/etc/shadow-" \
        "$MOUNT/etc/group-" \
        "$MOUNT/etc/gshadow-" \
        "$MOUNT/etc/resolv.conf" \
        "$MOUNT/var/log/alternatives.log" \
        "$MOUNT/var/log/apt/history.log" \
        "$MOUNT/var/log/apt/term.log" \
        "$MOUNT/var/log/dpkg.log"
}

pkg_install() {
    local CONTAINER="$1"
    local MOUNT="$(buildah mount "$CONTAINER")"
    shift

    if [ -n "$(find "$MOUNT/var/lib/apt/lists" -mindepth 0 -maxdepth 0 -empty)" ]; then
        cmd buildah run "$CONTAINER" -- \
            apt-get update
    fi

    cmd buildah run "$CONTAINER" -- \
        apt-get install --no-install-recommends --yes "$@"
}

pkg_remove() {
    local CONTAINER="$1"
    shift

    cmd buildah run "$CONTAINER" -- \
        apt-get remove --purge --autoremove --yes "$@"
}

pkg_version() {
    local CONTAINER="$1"
    local PACKAGE="$2"

    echo + "VERSION=\"\$(buildah run $(quote "$CONTAINER") -- dpkg -s $(quote "$PACKAGE")" \
        "| sed -ne 's/^Version: \(.*\)$/\1/p')\"" >&2
    buildah run "$CONTAINER" -- \
        dpkg -s "$PACKAGE" \
        | sed -ne 's/^Version: \(.*\)$/\1/p'
}

user_add() {
    local CONTAINER="$1"

    local NEW_USER="$2"
    local NEW_UID="$3"
    local NEW_HOME="${4:-/}"
    local NEW_SHELL="${5:-/sbin/nologin}"

    cmd buildah run "$CONTAINER" -- \
        adduser --uid "$NEW_UID" --shell "$NEW_SHELL" --disabled-password \
            --home "$NEW_HOME" --no-create-home --gecos "" \
            "$NEW_USER"
}

user_changeuid() {
    local CONTAINER="$1"
    local MOUNT="$(buildah mount "$CONTAINER")"

    local NEW_USER="$2"
    local NEW_UID="$3"
    local NEW_HOME="${4:-}"
    local NEW_SHELL="${5:-}"

    local NEW_USER_ESCAPED="$(printf '%s\n' "$NEW_USER" | sed 's/[][\.|$(){}?+*^]/\\&/g')"

    echo + "OLD_{UID,GID}=\"\$(grep $(quote "^$NEW_USER_ESCAPED:") …/etc/passwd | cut -d: -f{3,4})\"" >&2
    local OLD_UID="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f3)"
    local OLD_GID="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f4)"

    echo + "[ \"\$(sed -ne \"s/^\([^:]*\):[^:]*:\$OLD_GID:.*$/\1/p\" …/etc/group)\" == \"$NEW_USER\" ]" >&2
    local OLD_GROUP="$(sed -ne "s/^\([^:]*\):[^:]*:$OLD_GID:.*$/\1/p" "$MOUNT/etc/group")"
    if [ "$OLD_GROUP" != "$NEW_USER" ]; then
        echo "Primary group of user '$NEW_USER' is not group '$NEW_USER', but group '$OLD_GROUP'" >&2
        return 1
    fi

    local NEW_USER_ARGS=()
    [ -z "$NEW_HOME" ] || NEW_USER_ARGS+=( -d "$NEW_HOME" )
    [ -z "$NEW_SHELL" ] || NEW_USER_ARGS+=( -s "$NEW_SHELL" )

    cmd buildah run "$CONTAINER" -- \
        usermod -u "$NEW_UID" "${NEW_USER_ARGS[@]}" "$NEW_USER"

    cmd buildah run "$CONTAINER" -- \
        groupmod -g "$NEW_UID" "$NEW_USER"

    cmd buildah run "$CONTAINER" -- \
        find / \
            -path "/dev" -prune \
            -o -path "/proc" -prune \
            -o -path "/sys" -prune \
            -o -user "$OLD_UID" \
            -exec chown -h "$NEW_USER" {} \;

    cmd buildah run "$CONTAINER" -- \
        find / \
            -path "/dev" -prune \
            -o -path "/proc" -prune \
            -o -path "/sys" -prune \
            -o -group "$OLD_GID" \
            -exec chgrp -h "$NEW_USER" {} \;
}
