#!/bin/bash
# ci-tools -- containers/check-for-updates.sh
# Checks whether a container image must be rebuild or retagged.
#
# Copyright (c) 2021  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

set -eu -o pipefail
export LC_ALL=C

[ -n "${REGISTRY:-}" ] || { echo "Invalid build environment: Missing required env variable 'REGISTRY'" >&2; exit 1; }
[ -n "${OWNER:-}" ] || { echo "Invalid build environment: Missing required env variable 'OWNER'" >&2; exit 1; }
[ -n "${IMAGE:-}" ] || { echo "Invalid build environment: Missing required env variable 'IMAGE'" >&2; exit 1; }
[ -n "${TAGS:-}" ] || { echo "Invalid build environment: Missing required env variable 'TAGS'" >&2; exit 1; }

readarray -t -d' ' TAGS < <(printf '%s' "$TAGS")

SOURCE_TAG=""
if [ -n "${1:-}" ] && [ "$1" != "${TAGS[0]}" ]; then
    SOURCE_TAG="$1"
fi

# pull current image
echo + "IMAGE_ID=\"\$(podman pull $REGISTRY/$OWNER/$IMAGE:${SOURCE_TAG:-${TAGS[0]}})\"" >&2
IMAGE_ID="$(podman pull "$REGISTRY/$OWNER/$IMAGE:${SOURCE_TAG:-${TAGS[0]}}" || true)"

if [ -z "$IMAGE_ID" ]; then
    echo "Failed to pull image '$REGISTRY/$OWNER/$IMAGE:${SOURCE_TAG:-${TAGS[0]}}': No image with this tag found" >&2
    echo "Image rebuild required" >&2
    echo "build"
    exit
fi

# compare base image digests
echo + "BASE_IMAGE=\"\$(podman image inspect --format '{{index .Annotations \"org.opencontainers.image.base.name\"}}' $IMAGE_ID)\"" >&2
BASE_IMAGE="$(podman image inspect --format '{{index .Annotations "org.opencontainers.image.base.name"}}' "$IMAGE_ID" || true)"

if [ -z "$BASE_IMAGE" ]; then
    echo "Failed to inspect image '$REGISTRY/$OWNER/$IMAGE:${SOURCE_TAG:-${TAGS[0]}}': The image specifies no base image" >&2
    echo "Image rebuild required" >&2
    echo "build"
    exit
fi

echo + "BASE_IMAGE_DIGEST=\"\$(podman image inspect --format '{{index .Annotations \"org.opencontainers.image.base.digest\"}}' $IMAGE_ID)\"" >&2
BASE_IMAGE_DIGEST="$(podman image inspect --format '{{index .Annotations "org.opencontainers.image.base.digest"}}' "$IMAGE_ID" || true)"

echo + "LATEST_BASE_IMAGE_DIGEST=\"\$(skopeo inspect --format '{{.Digest}}' docker://$BASE_IMAGE)\"" >&2
LATEST_BASE_IMAGE_DIGEST="$(skopeo inspect --format '{{.Digest}}' "docker://$BASE_IMAGE" || true)"

if [ -z "$LATEST_BASE_IMAGE_DIGEST" ]; then
    echo "Failed to inspect latest base image '$BASE_IMAGE': \`skopeo inspect\` failed, likely there was no image with this tag found" >&2
    echo "Image rebuild required" >&2
    echo "build"
    exit
fi

if [ -z "$BASE_IMAGE_DIGEST" ] || [ "$BASE_IMAGE_DIGEST" != "$LATEST_BASE_IMAGE_DIGEST" ]; then
    echo "Base image digest mismatch, the image's base image '$BASE_IMAGE' is out of date" >&2
    echo "Current base image digest: $BASE_IMAGE_DIGEST" >&2
    echo "Latest base image digest: $LATEST_BASE_IMAGE_DIGEST" >&2
    echo "Image rebuild required" >&2
    echo "build"
    exit
fi

# also check generic tag
if [ -n "$SOURCE_TAG" ]; then
    echo + "IMAGE_ID=\"\$(podman pull $REGISTRY/$OWNER/$IMAGE:${TAGS[0]})\"" >&2
    IMAGE_ID="$(podman pull "$REGISTRY/$OWNER/$IMAGE:${TAGS[0]}" || true)"

    if [ -z "$IMAGE_ID" ]; then
        echo "Failed to pull image '$REGISTRY/$OWNER/$IMAGE:${TAGS[0]}': No image with this tag found" >&2
        echo "Image must be retagged" >&2
        echo "tag"
        exit
    fi

    echo + "BASE_IMAGE_DIGEST=\"\$(podman image inspect --format '{{index .Annotations \"org.opencontainers.image.base.digest\"}}' $IMAGE_ID)\"" >&2
    BASE_IMAGE_DIGEST="$(podman image inspect --format '{{index .Annotations "org.opencontainers.image.base.digest"}}' "$IMAGE_ID" || true)"

    if [ -z "$BASE_IMAGE_DIGEST" ] || [ "$BASE_IMAGE_DIGEST" != "$LATEST_BASE_IMAGE_DIGEST" ]; then
        echo "Base image digest mismatch, the image's base image '$BASE_IMAGE' is out of date" >&2
        echo "Current base image digest: $BASE_IMAGE_DIGEST" >&2
        echo "Latest base image digest: $LATEST_BASE_IMAGE_DIGEST" >&2
        echo "Image must be retagged" >&2
        echo "tag"
        exit
    fi
fi
