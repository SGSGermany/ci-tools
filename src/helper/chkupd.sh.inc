# ci-tools -- helper/chkupd.sh.inc
# Helper functions to check whether a container must be rebuild or retagged.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

chkupd_baseimage() {
    local IMAGE="$1"
    local GENERIC_TAG="$2"
    local SOURCE_TAG="${3:-}"

    [ "$SOURCE_TAG" != "$GENERIC_TAG" ] || SOURCE_TAG=""

    # pull current image
    echo + "IMAGE_ID=\"\$(podman pull $(quote "$IMAGE:${SOURCE_TAG:-$GENERIC_TAG}"))\"" >&2
    local IMAGE_ID="$(podman pull "$IMAGE:${SOURCE_TAG:-$GENERIC_TAG}" || true)"

    if [ -z "$IMAGE_ID" ]; then
        echo "Failed to pull image '$IMAGE:${SOURCE_TAG:-$GENERIC_TAG}': No image with this tag found" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    # compare base image digests
    echo + "BASE_IMAGE=\"\$(podman image inspect --format '{{index .Annotations \"org.opencontainers.image.base.name\"}}' $IMAGE_ID)\"" >&2
    local BASE_IMAGE="$(podman image inspect --format '{{index .Annotations "org.opencontainers.image.base.name"}}' "$IMAGE_ID" || true)"

    if [ -z "$BASE_IMAGE" ]; then
        echo "Failed to inspect image '$IMAGE:${SOURCE_TAG:-$GENERIC_TAG}': The image specifies no base image" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    echo + "BASE_IMAGE_DIGEST=\"\$(podman image inspect --format '{{index .Annotations \"org.opencontainers.image.base.digest\"}}' $IMAGE_ID)\"" >&2
    local BASE_IMAGE_DIGEST="$(podman image inspect --format '{{index .Annotations "org.opencontainers.image.base.digest"}}' "$IMAGE_ID" || true)"

    echo + "LATEST_BASE_IMAGE_DIGEST=\"\$(skopeo inspect --format '{{.Digest}}' $(quote "docker://$BASE_IMAGE"))\"" >&2
    local LATEST_BASE_IMAGE_DIGEST="$(skopeo inspect --format '{{.Digest}}' "docker://$BASE_IMAGE" || true)"

    if [ -z "$LATEST_BASE_IMAGE_DIGEST" ]; then
        echo "Failed to inspect latest base image '$BASE_IMAGE': \`skopeo inspect\` failed, likely there was no image with this tag found" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    if [ -z "$BASE_IMAGE_DIGEST" ] || [ "$BASE_IMAGE_DIGEST" != "$LATEST_BASE_IMAGE_DIGEST" ]; then
        echo "Base image digest mismatch, the image's base image '$BASE_IMAGE' is out of date" >&2
        echo "Current base image digest: $BASE_IMAGE_DIGEST" >&2
        echo "Latest base image digest: $LATEST_BASE_IMAGE_DIGEST" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    # also check generic tag
    if [ -n "$SOURCE_TAG" ]; then
        echo + "GENERIC_IMAGE_ID=\"\$(podman pull $IMAGE:$GENERIC_TAG)\"" >&2
        local GENERIC_IMAGE_ID="$(podman pull "$IMAGE:$GENERIC_TAG" || true)"

        if [ -z "$GENERIC_IMAGE_ID" ]; then
            echo "Failed to pull image '$IMAGE:$GENERIC_TAG': No image with this tag found" >&2
            echo "Image must be retagged" >&2
            echo "tag"
            return 1
        fi

        echo + "IMAGE_DIGEST=\"\$(podman image inspect --format '{{.Digest}}' $IMAGE_ID)\"" >&2
        local IMAGE_DIGEST="$(podman image inspect --format '{{.Digest}}' "$IMAGE_ID")"

        echo + "GENERIC_IMAGE_DIGEST=\"\$(podman image inspect --format '{{.Digest}}' $GENERIC_IMAGE_ID)\"" >&2
        local GENERIC_IMAGE_DIGEST="$(podman image inspect --format '{{.Digest}}' "$GENERIC_IMAGE_ID")"

        if [ "$IMAGE_DIGEST" != "$GENERIC_IMAGE_DIGEST" ]; then
            echo "Image digest mismatch, the generic tag '$GENERIC_TAG' references a different image than '$SOURCE_TAG'" >&2
            echo "Source image digest: $IMAGE_DIGEST" >&2
            echo "Generic image digest: $GENERIC_IMAGE_DIGEST" >&2
            echo "Image must be retagged" >&2
            echo "tag"
            return 1
        fi

        echo + "BASE_IMAGE_DIGEST=\"\$(podman image inspect --format '{{index .Annotations \"org.opencontainers.image.base.digest\"}}' $GENERIC_IMAGE_ID)\"" >&2
        local BASE_IMAGE_DIGEST="$(podman image inspect --format '{{index .Annotations "org.opencontainers.image.base.digest"}}' "$GENERIC_IMAGE_ID" || true)"

        if [ -z "$BASE_IMAGE_DIGEST" ] || [ "$BASE_IMAGE_DIGEST" != "$LATEST_BASE_IMAGE_DIGEST" ]; then
            echo "Base image digest mismatch, the image's base image '$BASE_IMAGE' is out of date" >&2
            echo "Current base image digest: $BASE_IMAGE_DIGEST" >&2
            echo "Latest base image digest: $LATEST_BASE_IMAGE_DIGEST" >&2
            echo "Image must be retagged" >&2
            echo "tag"
            return 1
        fi
    fi
}
