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

con_build() {
    local CONTEXT="${@: -2:1}"
    local CONTEXT_INFO="${@: -1}"
    set -- "${@:1:$#-2}"

    echo + "[ -f $(quote "$CONTEXT_INFO/Dockerfile") ] || [ -f $(quote "$CONTEXT_INFO/Containerfile") ]" >&2
    if [ ! -f "$CONTEXT/Dockerfile" ] && [ ! -f "$CONTEXT/Containerfile" ]; then
        echo "Invalid build context: Dockerfile '$CONTEXT_INFO/Dockerfile' not found" >&2
        return 1
    fi

    local ORIG_BASE_IMAGE_PATTERN
    local ORIG_BASE_IMAGE

    local PARAMS=()
    while [ $# -gt 0 ]; do
        case "$1" in
            "--check-from")
                if [ $# -lt 2 ]; then
                    echo "Missing required argument for option '--check-from': IMAGE_PATTERN" >&2
                    return 1
                fi

                ORIG_BASE_IMAGE_PATTERN="$2"
                shift 2
                ;;

            *)
                PARAMS+=( "$1" )
                shift
                ;;
        esac
    done

    if [ -n "$ORIG_BASE_IMAGE_PATTERN" ]; then
        local FILE="Dockerfile"
        [ -f "$CONTEXT/Dockerfile" ] || FILE="Containerfile"

        echo + "ORIG_BASE_IMAGE=\"\$(sed -ne '0,/^FROM /{s/^FROM \(\S*\)\( as \S*\)\?$/\1/pi}' $(quote "$CONTEXT_INFO/$FILE"))\"" >&2
        ORIG_BASE_IMAGE="$(sed -ne '0,/^FROM /{s/^FROM \(\S*\)\( as \S*\)\?$/\1/pi}' "$CONTEXT/$FILE")"

        echo + "[[ $(quote "$ORIG_BASE_IMAGE") == $ORIG_BASE_IMAGE_PATTERN ]]"
        if ! [[ "$ORIG_BASE_IMAGE" == $ORIG_BASE_IMAGE_PATTERN ]]; then
            echo "Invalid build context: Expecting base image in '$CONTEXT_INFO/$FILE' to match '$ORIG_BASE_IMAGE_PATTERN', got '$ORIG_BASE_IMAGE'" >&2
            return 1
        fi
    fi

    echo + "buildah bud $(quote "${PARAMS[@]}") $(quote "$CONTEXT_INFO")" >&2
    buildah bud "${PARAMS[@]}" "$CONTEXT"
}

con_commit() {
    local CONTAINER="$1"
    local TAGS=( "${@:2}" )

    cmd buildah commit "$CONTAINER" "localhost/$IMAGE:${TAGS[0]}"
    cmd buildah rm "$CONTAINER"

    for TAG in "${TAGS[@]:1}"; do
        cmd podman tag "localhost/$IMAGE:${TAGS[0]}" "localhost/$IMAGE:$TAG"
    done
}
