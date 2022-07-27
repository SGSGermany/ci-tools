# ci-tools -- helper/chkupd-alpine.sh.inc
# Helper functions to check whether Alpine Linux package upgrades are available.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

source "$(dirname "${BASH_SOURCE[0]}")/common-traps.sh.inc"

__apk_info() {
    local CONTAINER="$1"

    local PACKAGES PACKAGE_VERSIONS
    readarray -t PACKAGES < <(buildah run "$CONTAINER" -- apk info)
    readarray -t PACKAGE_VERSIONS < <(buildah run "$CONTAINER" -- apk info -v)

    local INDEX PACKAGE
    for (( INDEX=0 ; INDEX < ${#PACKAGES[@]} ; INDEX++ )); do
        if [ "${PACKAGE_VERSIONS[$INDEX]:0:${#PACKAGES[$INDEX]}}" != "${PACKAGES[$INDEX]}" ]; then
            return 1
        fi

        echo "${PACKAGES[$INDEX]} ${PACKAGE_VERSIONS[$INDEX]:${#PACKAGES[$INDEX]}+1}"
    done
}

chkupd_packages() {
    local BASE_IMAGE="$1"
    local IMAGE="$2"
    shift 2

    echo + "BASE_CONTAINER=\"\$(buildah from $(quote "$BASE_IMAGE"))\"" >&2
    local BASE_CONTAINER="$(buildah from "$BASE_IMAGE" || true)"

    if [ -z "$BASE_CONTAINER" ]; then
        echo "Failed to pull base image '$BASE_IMAGE': No image with this tag found" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    trap_exit buildah rm "$BASE_CONTAINER"

    # run `apk update` to make sure that we don't confound pending base image updates
    # and `apk info` to get a list of installed packages and their respective versions
    cmd buildah run "$BASE_CONTAINER" -- \
        apk update >&2

    echo + "BASE_PACKAGES=\"\$(buildah run $(quote "$BASE_CONTAINER") -- apk info -v | sort)\"" >&2
    local BASE_PACKAGES="$(__apk_info "$BASE_CONTAINER" | sort)"

    # run `apk add` to install the container's packages, then run `apk info` again
    # and compare it with the previous list to get a list of the newly installed packages
    cmd buildah run "$BASE_CONTAINER" -- \
        apk add "$@" >&2

    echo + "ALL_PACKAGES=\"\$(buildah run $(quote "$BASE_CONTAINER") -- apk info -v | sort)\"" >&2
    local ALL_PACKAGES="$(__apk_info "$BASE_CONTAINER" | sort)"

    echo + "readarray -t PACKAGES < <(comm -13 <(echo \"\$BASE_PACKAGES\") <(echo \"\$ALL_PACKAGES\"))" >&2
    readarray -t PACKAGES < <(comm -13 <(echo "$BASE_PACKAGES") <(echo "$ALL_PACKAGES"))

    # pull current image
    echo + "CONTAINER=\"\$(buildah from $(quote "$IMAGE"))\"" >&2
    local CONTAINER="$(buildah from "$IMAGE" || true)"

    if [ -z "$CONTAINER" ]; then
        echo "Failed to pull image '$IMAGE': No image with this tag found" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    trap_exit buildah rm "$CONTAINER"

    # run `apk info` to get a list of installed packages
    echo + "OLD_PACKAGES=\"\$(buildah run $(quote "$CONTAINER") -- apk info -v | sort)\"" >&2
    local OLD_PACKAGES="$(__apk_info "$BASE_CONTAINER" | sort)"

    # check whether the container's packages are up-to-date
    echo + "PACKAGE_UPGRADES=()" >&2
    echo + "for PACKAGE_DATA in \"\${PACKAGES[@]}\"; do" >&2
    echo + "    PACKAGE=\"\$(cut -d' ' -f1 <<< \"\$PACKAGE_DATA\")\"" >&2
    echo + "    NEW_VERSION=\"\$(cut -d' ' -f2 <<< \"\$PACKAGE_DATA\")\"" >&2
    echo + "    OLD_VERSION=\"\$(grep \"^\$PACKAGE \" <<< \"\$OLD_PACKAGES\" | cut -d' ' -f2)\"" >&2
    echo + "    [ \"\$OLD_VERSION\" == \"\$NEW_VERSION\" ] || PACKAGE_UPGRADES+=( … )" >&2
    echo + "done" >&2

    local PACKAGE_UPGRADES=()
    local PACKAGE= PACKAGE_ESCAPED= NEW_VERSION= OLD_VERSION=
    for PACKAGE_DATA in "${PACKAGES[@]}"; do
        PACKAGE="$(cut -d' ' -f1 <<< "$PACKAGE_DATA")"
        PACKAGE_ESCAPED="$(printf '%s\n' "$PACKAGE" | sed 's/[][\.|$(){}?+*^]/\\&/g')"

        NEW_VERSION="$(cut -d' ' -f2 <<< "$PACKAGE_DATA")"
        OLD_VERSION="$(grep "^$PACKAGE_ESCAPED " <<< "$OLD_PACKAGES" | cut -d' ' -f2 || true)"
        if [ -z "$OLD_VERSION" ] || [ "$OLD_VERSION" != "$NEW_VERSION" ]; then
            PACKAGE_UPGRADES+=( "$PACKAGE $OLD_VERSION -> $NEW_VERSION" )
        fi
    done

    if [ ${#PACKAGE_UPGRADES[@]} -gt 0 ]; then
        echo "Image is out of date: Package upgrades are available" >&2
        printf '%s\n' "${PACKAGE_UPGRADES[@]}" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi
}
