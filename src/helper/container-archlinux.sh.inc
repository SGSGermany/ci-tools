# ci-tools -- helper/container-alpine.sh.inc
# Helper functions for @SGSGermany's containers based on Arch Linux.
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
        pacman -Sc --noconfirm

    echo + "trunc …/var/cache/pacman/pkg …/var/log/{lastlog,wtmp,btmp} …/var/log/pacman.log" >&2
    trunc \
        "$MOUNT/var/cache/pacman/pkg" \
        "$MOUNT/var/log/btmp" \
        "$MOUNT/var/log/lastlog" \
        "$MOUNT/var/log/pacman.log" \
        "$MOUNT/var/log/wtmp"

    echo + "rm -f …/etc/{passwd,shadow}- …/etc/{group,gshadow}- …/etc/resolv.conf" >&2
    rm -f \
        "$MOUNT/etc/passwd-" \
        "$MOUNT/etc/shadow-" \
        "$MOUNT/etc/group-" \
        "$MOUNT/etc/gshadow-" \
        "$MOUNT/etc/resolv.conf"
}

pkg_install() {
    local CONTAINER="$1"
    shift

    local PKG_STATUS=0
    cmd buildah run "$CONTAINER" -- \
        pacman -S --noconfirm "$@" \
            || { PKG_STATUS=$?; true; }

    if [ $PKG_STATUS -ne 0 ]; then
        cmd buildah run "$CONTAINER" -- \
            pacman -Syu --noconfirm

        cmd buildah run "$CONTAINER" -- \
            pacman -S --noconfirm "$@"
    fi
}

pkg_remove() {
    local CONTAINER="$1"
    shift

    cmd buildah run "$CONTAINER" -- \
        pacman -Rsn --noconfirm "$@"
}

pkg_version() {
    local CONTAINER="$1"
    local PACKAGE="$2"

    echo + "VERSION=\"\$(buildah run $(quote "$CONTAINER") -- pacman -Si $(quote "$PACKAGE")" \
        "| sed -ne 's/^Version\s*: \(.*\)$/\1/p')\"" >&2
    buildah run "$CONTAINER" -- \
        pacman -Si "$PACKAGE" \
        | sed -ne 's/^Version\s*: \(.*\)$/\1/p'
}

user_add() {
    local CONTAINER="$1"

    local NEW_USER="$2"
    local NEW_UID="$3"
    local NEW_HOME="${4:-/}"
    local NEW_SHELL="${5:-/sbin/nologin}"

    cmd buildah run "$CONTAINER" -- \
        groupadd -g "$NEW_UID" "$NEW_USER"

    cmd buildah run "$CONTAINER" -- \
        useradd -u "$NEW_UID" -g "$NEW_USER" -s "$NEW_SHELL" -d "$NEW_HOME" -M -c "" "$NEW_USER"
}

user_changeuid() {
    local CONTAINER="$1"
    local MOUNT="$(buildah mount "$CONTAINER")"

    local NEW_USER="$2"
    local NEW_UID="$3"

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

    cmd buildah run "$CONTAINER" -- \
        usermod -u "$NEW_UID" "$NEW_USER"

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
