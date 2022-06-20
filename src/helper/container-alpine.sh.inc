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

    echo + "trunc …/run …/tmp …/var/cache/apk …/var/log/{lastlog,wtmp,btmp} …/var/tmp"
    trunc \
        "$MOUNT/run" \
        "$MOUNT/tmp" \
        "$MOUNT/var/cache/apk" \
        "$MOUNT/var/log/btmp" \
        "$MOUNT/var/log/lastlog" \
        "$MOUNT/var/log/wtmp" \
        "$MOUNT/var/tmp"

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
    shift

    cmd buildah run "$CONTAINER" -- \
        apk add "$@"
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

    local NEW_USER="$2"
    local NEW_UID="$3"

    local NEW_USER_ESCAPED="$(printf '%s\n' "$NEW_USER" | sed 's/[][\.|$(){}?+*^]/\\&/g')"

    echo + "OLD_{UID,GID,GECOS,HOME,SHELL}=\"\$(grep $(quote "^$NEW_USER_ESCAPED:") …/etc/passwd | cut -d: -f{3..7})\""
    local OLD_UID="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f3)"
    local OLD_GID="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f4)"
    local OLD_GECOS="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f5)"
    local OLD_HOME="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f6)"
    local OLD_SHELL="$(grep "^$NEW_USER_ESCAPED:" "$MOUNT/etc/passwd" | cut -d: -f7)"

    cmd buildah run "$CONTAINER" -- \
        deluser "$NEW_USER"

    cmd buildah run "$CONTAINER" -- \
        adduser -u "$NEW_UID" -s "$OLD_SHELL" -D -h "$OLD_HOME" -H -g "$OLD_GECOS" "$NEW_USER"

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
