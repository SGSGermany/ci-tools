#!/bin/bash
# ci-tools -- containers/get-metadata.sh
# Returns the metadata of a (possibly not yet pulled) container image.
#
# Copyright (c) 2021  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

set -e
export LC_ALL=C

REGISTRY="$1"
IMAGE="$2"
if [ -z "$REGISTRY" ]; then
    echo "No container registry given" >&2
    exit 1
elif [ -z "$IMAGE" ]; then
    echo "No container image given" >&2
    exit 1
fi

echo + "podman image exists $IMAGE" >&2
if ! podman image exists "$IMAGE"; then
    echo + "podman pull $REGISTRY/$IMAGE" >&2
    podman pull "$REGISTRY/$IMAGE"
fi

echo + "podman image inspect $IMAGE" >&2
podman image inspect "$IMAGE"
