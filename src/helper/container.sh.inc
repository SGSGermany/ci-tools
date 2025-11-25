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

source "$(dirname "${BASH_SOURCE[0]}")/common-traps.sh.inc"

con_build() {
    local CONTEXT="${@: -2:1}"
    local CONTEXT_INFO="${@: -1}"
    set -- "${@:1:$#-2}"

    local IMAGE=
    local ORIG_BASE_IMAGE_PATTERN=
    local ORIG_BASE_IMAGE=
    local FILE=
    local FILE_INFO=
    local OMIT_HISTORY=
    local PARAMS=()

    while [ $# -gt 0 ]; do
        case "$1" in
            "-t"|"--tag")
                [ $# -gt 1 ] || { echo "Missing required argument for option '--tag'" >&2; return 1; }
                IMAGE="$2"
                shift 2
                ;;

            "--check-from")
                [ $# -gt 1 ] || { echo "Missing required argument for option '--check-from'" >&2; return 1; }
                ORIG_BASE_IMAGE_PATTERN="$2"
                shift 2
                ;;

            "-f"|"--file")
                [ $# -gt 1 ] || { echo "Missing required argument for option '--file'" >&2; return 1; }
                [ -n "$FILE" ] || FILE="$2"
                PARAMS+=( --file "$2" )
                shift 2
                ;;

            "--omit-history")
                [ $# -gt 1 ] || { echo "Missing required argument for option '--omit-history'" >&2; return 1; }
                [ "$2" == "true" ] || [ "$2" == "false" ] \
                    || { echo "Invalid argument for option '--omit-history': $2" >&2; return 1; }
                OMIT_HISTORY="$2"
                shift 2
                ;;

            *)
                PARAMS+=( "$1" )
                shift
                ;;
        esac
    done

    [ -n "$IMAGE" ] || { echo "Missing required option: --tag" >&2; return 1; }

    PARAMS+=( --tag "$IMAGE" )
    PARAMS+=( --omit-history="${OMIT_HISTORY:-true}" )

    if [ -n "$FILE" ]; then
        if [ -f "$FILE" ]; then
            FILE_INFO="$FILE"
        elif [ "${FILE:0:1}" != "/" ] && [ -f "$CONTEXT/$FILE" ]; then
            FILE="$CONTEXT/$FILE"
            FILE_INFO="$CONTEXT_INFO/$FILE"
        else
            echo "Invalid build context: Containerfile '$FILE' not found" >&2
            return 1
        fi
    elif [ -f "$CONTEXT/Containerfile" ]; then
        FILE="$CONTEXT/Containerfile"
        FILE_INFO="$CONTEXT_INFO/Containerfile"
    elif [ -f "$CONTEXT/Dockerfile" ]; then
        FILE="$CONTEXT/Dockerfile"
        FILE_INFO="$CONTEXT_INFO/Dockerfile"
    else
        echo "Invalid build context: Containerfile '$CONTEXT_INFO/Containerfile' not found" >&2
        return 1
    fi

    if [ -n "$ORIG_BASE_IMAGE_PATTERN" ]; then
        echo + "ORIG_BASE_IMAGE=\"\$(sed -ne '0,/^FROM /{s/^FROM \(\S*\)\( as \S*\)\?$/\1/pi}' $(quote "$FILE_INFO"))\"" >&2
        ORIG_BASE_IMAGE="$(sed -ne '0,/^FROM /{s/^FROM \(\S*\)\( as \S*\)\?$/\1/pi}' "$FILE")"

        echo + "[[ $(quote "$ORIG_BASE_IMAGE") == $ORIG_BASE_IMAGE_PATTERN ]]"
        if ! [[ "$ORIG_BASE_IMAGE" == $ORIG_BASE_IMAGE_PATTERN ]]; then
            echo "Invalid build context: Expecting base image in '$FILE_INFO' to match '$ORIG_BASE_IMAGE_PATTERN', got '$ORIG_BASE_IMAGE'" >&2
            return 1
        fi
    fi

    echo + "buildah bud $(quote "${PARAMS[@]}") $(quote "$CONTEXT_INFO")" >&2
    buildah bud "${PARAMS[@]}" "$CONTEXT"

    trap_exit podman rmi "$IMAGE"
}

con_cleanup() {
    local CONTAINER="$1"

    local CLEANUP_CONFIG=()
    while IFS= read -r ANNOTATION; do
        if [[ "$ANNOTATION" == "org.opencontainers.image."* ]]; then
            CLEANUP_CONFIG+=( --annotation "$ANNOTATION"- )
        elif [[ "$ANNOTATION" == "com.docker.official-images."* ]]; then
            CLEANUP_CONFIG+=( --annotation "$ANNOTATION"- )
        fi
    done < <(buildah inspect --format '{{ range $k, $_ := .ImageAnnotations }}{{ println $k }}{{ end }}' "$CONTAINER")

    if [ "${#CLEANUP_CONFIG[@]}" -gt 0 ]; then
        cmd buildah config "${CLEANUP_CONFIG[@]}" "$CONTAINER"
    fi
}

con_commit() {
    local CONTAINER="$1"
    local IMAGE="$2"
    local TAGS=( "${@:3}" )
    [ ${#TAGS[@]} -gt 0 ] || TAGS=( "latest" )

    cmd buildah commit --omit-history "$CONTAINER" "localhost/$IMAGE:${TAGS[0]}"
    cmd buildah rm "$CONTAINER"

    for TAG in "${TAGS[@]:1}"; do
        cmd podman tag "localhost/$IMAGE:${TAGS[0]}" "localhost/$IMAGE:$TAG"
    done
}
