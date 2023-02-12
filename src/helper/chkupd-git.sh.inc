# ci-tools -- helper/chkupd-git.sh.inc
# Helper functions to check whether a new version tag was pushed to a Git repo.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

source "$(dirname "${BASH_SOURCE[0]}")/common-traps.sh.inc"
source "$(dirname "${BASH_SOURCE[0]}")/git.sh.inc"

chkupd_git() {
    local IMAGE="$1"
    local GIT_REPO="$2"
    local GIT_TAG_PATTERN="$3"
    local VERSION_FILE="$4"

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

    # compare current and latest Git tags
    echo + "CURRENT_VERSION=\"\$(buildah run --user root $(quote "$CONTAINER") --" \
        "sed -ne 's/^VERSION=\(.*\)$/\1/p' $(quote "$VERSION_FILE"))\"" >&2
    local CURRENT_VERSION="$(buildah run --user root "$CONTAINER" -- \
        sed -ne 's/^VERSION=\(.*\)$/\1/p' "$VERSION_FILE")"

    local VERSION="$(git_latest "$GIT_REPO" "$GIT_TAG_PATTERN")"

    if [ "$CURRENT_VERSION" != "$VERSION" ]; then
        echo "Image is out of date: New Git tag is available" >&2
        echo "Current Git tag: $CURRENT_VERSION" >&2
        echo "Latest Git tag: $VERSION" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi
}
