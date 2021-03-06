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

set -eu -o pipefail
export LC_ALL=C

[ -v CI_TOOLS ] && [ "$CI_TOOLS" == "SGSGermany" ] \
    || { echo "Invalid build environment: Environment variable 'CI_TOOLS' not set or invalid" >&2; exit 1; }

[ -v CI_TOOLS_PATH ] && [ -d "$CI_TOOLS_PATH" ] \
    || { echo "Invalid build environment: Environment variable 'CI_TOOLS_PATH' not set or invalid" >&2; exit 1; }

source "$CI_TOOLS_PATH/helper/common.sh.inc"

REGISTRY="${1:-}"
if [ -z "$REGISTRY" ]; then
    echo "Missing required parameter 'REGISTRY'" >&2
    exit 1
fi

IMAGE="${2:-}"
if [ -z "$IMAGE" ]; then
    echo "Missing required parameter 'IMAGE'" >&2
    exit 1
fi

echo + "podman image exists $(quote "localhost/$IMAGE")" >&2
if ! podman image exists "localhost/$IMAGE"; then
    echo + "podman pull $(quote "$REGISTRY/$IMAGE")" >&2
    podman pull "$REGISTRY/$IMAGE" >&2

    echo + "podman image inspect $(quote "$REGISTRY/$IMAGE")" >&2
    podman image inspect "$REGISTRY/$IMAGE"
else
    echo + "podman image inspect $(quote "localhost/$IMAGE")" >&2
    podman image inspect "localhost/$IMAGE"
fi
