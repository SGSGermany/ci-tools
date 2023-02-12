# ci-tools -- helper/chkupd-pypi.sh.inc
# Helper functions to check whether PyPi package upgrades are available.
#
# Copyright (c) 2023  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

source "$(dirname "${BASH_SOURCE[0]}")/common-traps.sh.inc"

chkupd_pypi() {
    local IMAGE="$1"
    shift

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

    # check whether image is based on Alpine Linux
    echo + "if buildah run $(quote "$CONTAINER") -- test -f /etc/alpine-release; then … fi" >&2
    if buildah run "$CONTAINER" -- test -f "/etc/alpine-release"; then
        echo "Invalid image '$IMAGE': Expecting an Alpine Linux image" >&2
        echo "Internal error" >&2
        echo "build"
        return 2
    fi

    # install pip
    cmd buildah run "$CONTAINER" -- \
        apk add --no-cache py3-pip >&2

    # check for PyPi package upgrades
    echo + "PACKAGE_UPGRADES=\"\$(buildah run $(quote "$CONTAINER") -- pip list --user --outdated)\"" >&2
    local PACKAGE_UPGRADES="$(buildah run "$CONTAINER" -- pip list --user --outdated)"

    if [ -n "$PACKAGE_UPGRADES" ]; then
        echo "Image is out of date: PyPi package upgrades are available" >&2
        echo "$PACKAGE_UPGRADES" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi
}
