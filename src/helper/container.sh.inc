# ci-tools -- helper/container.sh.inc
# Common helper functions for @SGSGermany's container build scripts.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

con_commit() {
    local CONTAINER="$1"
    local TAGS=( "${@:2}" )

    cmd buildah commit "$CONTAINER" "localhost/$IMAGE:${TAGS[0]}"
    cmd buildah rm "$CONTAINER"

    for TAG in "${TAGS[@]:1}"; do
        cmd podman tag "localhost/$IMAGE:${TAGS[0]}" "localhost/$IMAGE:$TAG"
    done
}
