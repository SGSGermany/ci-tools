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
    echo + "if buildah run --user root $(quote "$CONTAINER") -- test -f /etc/alpine-release; then … fi" >&2
    if buildah run --user root "$CONTAINER" -- test -f "/etc/alpine-release"; then
        echo "Invalid image '$IMAGE': Expecting an Alpine Linux image" >&2
        echo "Internal error" >&2
        echo "build"
        return 2
    fi

    # install pip
    cmd buildah run --user root "$CONTAINER" -- \
        apk add --no-cache py3-pip >&2

    # check for PyPi package upgrades
    echo + "PACKAGE_UPGRADES=\"\$(buildah run --user root $(quote "$CONTAINER") -- pip list --user --outdated)\"" >&2
    local PACKAGE_UPGRADES="$(buildah run --user root "$CONTAINER" -- pip list --user --outdated)"

    if [ -n "$PACKAGE_UPGRADES" ]; then
        echo "Image is out of date: PyPi package upgrades are available" >&2
        echo "$PACKAGE_UPGRADES" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi
}

chkupd_pypi_requirements() {
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
    echo + "if buildah run --user root $(quote "$CONTAINER") -- test -f /etc/alpine-release; then … fi" >&2
    if buildah run --user root "$CONTAINER" -- test -f "/etc/alpine-release"; then
        echo "Invalid image '$IMAGE': Expecting an Alpine Linux image" >&2
        echo "Internal error" >&2
        echo "build"
        return 2
    fi

    # install pip and jq
    cmd buildah run --user root "$CONTAINER" -- \
        apk add --no-cache py3-pip jq >&2

    # check for PyPi package upgrades based on requirements.txt files
    echo + "PACKAGE_UPGRADES=()" >&2
    echo + "for REQUIREMENTS_FILE in \"\$@\"; do" >&2
    echo + "    readarray -t -O \${#PACKAGE_UPGRADES[@]} PACKAGE_UPGRADES < <( \\" >&2
    echo + "        buildah run --user root $(quote "$CONTAINER") -- \\" >&2
    echo + "            pip install --upgrade -r \"\$REQUIREMENTS_FILE\" --dry-run --quiet --report - 2> /dev/null \\" >&2
    echo + "                | jq -r '.install[] | \"\(.metadata.name)  \(.metadata.version)\"' \\" >&2
    echo + "    )" >&2
    echo + "done" >&2

    local PACKAGE_UPGRADES=()
    for REQUIREMENTS_FILE in "$@"; do
        readarray -t -O "${#PACKAGE_UPGRADES[@]}" PACKAGE_UPGRADES < <( \
            buildah run --user root "$CONTAINER" -- \
                pip install --upgrade -r "$REQUIREMENTS_FILE" --dry-run --quiet --report - 2> /dev/null \
                    | jq -r '.install[] | "\(.metadata.name)  \(.metadata.version)"' \
        )
    done

    if [ ${#PACKAGE_UPGRADES[@]} -gt 0 ]; then
        echo "Image is out of date: PyPi package upgrades are available" >&2
        printf '%s\n' "${PACKAGE_UPGRADES[@]}" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi
}
