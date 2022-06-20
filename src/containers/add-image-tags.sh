#!/bin/bash
# ci-tools -- containers/add-image-tags.sh
# Adds additional tags to an existing container image.
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

[ -v CI_TOOLS ] && [ "$CI_TOOLS" == "SGSGermany" ] \
    || { echo "Invalid build environment: Environment variable 'CI_TOOLS' not set or invalid" >&2; exit 1; }

[ -v CI_TOOLS_PATH ] && [ -d "$CI_TOOLS_PATH" ] \
    || { echo "Invalid build environment: Environment variable 'CI_TOOLS_PATH' not set or invalid" >&2; exit 1; }

[ -n "${REGISTRY:-}" ] || { echo "Invalid build environment: Missing required env variable 'REGISTRY'" >&2; exit 1; }
[ -n "${OWNER:-}" ] || { echo "Invalid build environment: Missing required env variable 'OWNER'" >&2; exit 1; }
[ -n "${IMAGE:-}" ] || { echo "Invalid build environment: Missing required env variable 'IMAGE'" >&2; exit 1; }
[ -n "${TAGS:-}" ] || { echo "Invalid build environment: Missing required env variable 'TAGS'" >&2; exit 1; }

readarray -t -d' ' TAGS < <(printf '%s' "$TAGS")

SOURCE_TAG="${1:-}"
if [ -z "$SOURCE_TAG" ]; then
    echo "Missing required parameter 'SOURCE_TAG'" >&2
    exit 1
fi

echo + "podman image exists $IMAGE:$SOURCE_TAG" >&2
if ! podman image exists "$IMAGE:$SOURCE_TAG"; then
    echo "Invalid image '$IMAGE:$SOURCE_TAG': No image with this tag found" >&2
    exit 1
fi

for TAG in "${TAGS[@]}"; do
    echo + "podman tag $IMAGE:$SOURCE_TAG $IMAGE:$TAG" >&2
    podman tag "$IMAGE:$SOURCE_TAG" "$IMAGE:$TAG"
done
