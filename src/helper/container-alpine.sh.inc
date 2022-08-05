# ci-tools -- helper/container-alpine.sh.inc
# Helper functions for @SGSGermany's containers based on Alpine Linux.
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

    echo + "trunc …/var/cache/apk …/var/log/{lastlog,wtmp,btmp}"
    trunc \
        "$MOUNT/var/cache/apk" \
        "$MOUNT/var/log/btmp" \
        "$MOUNT/var/log/lastlog" \
        "$MOUNT/var/log/wtmp"

    echo + "rm -f …/etc/{passwd,shadow}- …/etc/{group,gshadow}- …/etc/resolv.conf"
    rm -f \
        "$MOUNT/etc/passwd-" \
        "$MOUNT/etc/shadow-" \
        "$MOUNT/etc/group-" \
        "$MOUNT/etc/gshadow-" \
        "$MOUNT/etc/resolv.conf"
}

pkg_install() {
    local CONTAINER="$1"
    local MOUNT="$(buildah mount "$CONTAINER")"
    shift

    if [ -n "$(find "$MOUNT/var/cache/apk" -mindepth 0 -maxdepth 0 -empty)" ]; then
        cmd buildah run "$CONTAINER" -- \
            apk update
    fi

    cmd buildah run "$CONTAINER" -- \
        apk add "$@"
}

pkg_remove() {
    local CONTAINER="$1"
    shift

    cmd buildah run "$CONTAINER" -- \
        apk del "$@"
}

pkg_version() {
    local CONTAINER="$1"
    local PACKAGE="$2"
    local PACKAGE_ESCAPED=$(printf '%s\n' "$PACKAGE" | sed -e 's/[]\/$*.^[]/\\&/g');

    echo + "VERSION=\"\$(buildah run $(quote "$CONTAINER") --" \
        "apk search --no-cache --exact $(quote "$PACKAGE")" \
        "| sed -ne $(quote "s/^$PACKAGE_ESCAPED-\(.*\)$/\1/p"))\"" >&2
    local VERSION="$(buildah run "$CONTAINER" -- \
        apk search --no-cache --exact "$PACKAGE" \
        | sed -ne "s/^$PACKAGE_ESCAPED-\(.*\)$/\1/p")"

    [ -n "$VERSION" ] || { echo "Package '$PACKAGE' not found" >&2; return 1; }
    echo "$VERSION"
}

user_add() {
    local CONTAINER="$1"

    local NEW_USER="$2"
    local NEW_UID="$3"
    local NEW_HOME="${4:-/}"
    local NEW_SHELL="${5:-/sbin/nologin}"

    cmd buildah run "$CONTAINER" -- \
        adduser -u "$NEW_UID" -s "$NEW_SHELL" -D -h "$NEW_HOME" -H -g "" "$NEW_USER"
}

user_changeuid() {
    local CONTAINER="$1"
    local MOUNT="$(buildah mount "$CONTAINER")"

    local NEW_USER="$2"
    local NEW_UID="$3"
    local NEW_HOME="${4:-}"
    local NEW_SHELL="${5:-}"

    local NEW_USER_ESCAPED="$(printf '%s\n' "$NEW_USER" | sed 's/[][\.|$(){}?+*^]/\\&/g')"

    echo + "OLD_{UID,GID,GECOS,HOME,SHELL}=\"\$(grep $(quote "^$NEW_USER_ESCAPED:") …/etc/passwd | cut -d: -f{3..7})\""
    local OLD_UID="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f3)"
    local OLD_GID="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f4)"
    local OLD_GECOS="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f5)"
    local OLD_HOME="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f6)"
    local OLD_SHELL="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f7)"

    echo + "[ \"\$(sed -ne \"s/^\([^:]*\):[^:]*:\$OLD_GID:.*$/\1/p\" …/etc/group)\" == \"$NEW_USER\" ]" >&2
    local OLD_GROUP="$(sed -ne "s/^\([^:]*\):[^:]*:$OLD_GID:.*$/\1/p" "$MOUNT/etc/group")"
    if [ "$OLD_GROUP" != "$NEW_USER" ]; then
        echo "Primary group of user '$NEW_USER' is not group '$NEW_USER', but group '$OLD_GROUP'" >&2
        return 1
    fi

    cmd buildah run "$CONTAINER" -- \
        deluser "$NEW_USER"

    cmd buildah run "$CONTAINER" -- \
        adduser -u "$NEW_UID" \
            -s "${NEW_SHELL:-$OLD_SHELL}" -D \
            -h "${NEW_HOME:-$OLD_HOME}" -H \
            -g "$OLD_GECOS" \
            "$NEW_USER"

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
