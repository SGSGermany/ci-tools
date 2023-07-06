#!/bin/bash
# ci-tools -- containers/generate-image-tags.sh
# Generates a list of tags for container images.
#
# Copyright (c) 2021  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

set -eu -o pipefail
export LC_ALL=C.UTF-8

[ -v CI_TOOLS ] && [ "$CI_TOOLS" == "SGSGermany" ] \
    || { echo "Invalid build environment: Environment variable 'CI_TOOLS' not set or invalid" >&2; exit 1; }

[ -v CI_TOOLS_PATH ] && [ -d "$CI_TOOLS_PATH" ] \
    || { echo "Invalid build environment: Environment variable 'CI_TOOLS_PATH' not set or invalid" >&2; exit 1; }

source "$CI_TOOLS_PATH/helper/common.sh.inc"

[ -x "$(which git 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: git" >&2; exit 1; }

git rev-parse --git-dir > /dev/null 2>&1 \
    || { echo "Invalid build environment: This is no Git repository" >&2; exit 1; }

git_ls_tags() {
    git ls-remote "origin" "refs/tags/$1" | cut -f2 | sort_semver
}

GIT_REF="${1:-}"
if [ -z "$GIT_REF" ]; then
    echo "Missing required parameter 'GIT_REF'" >&2
    exit 1
fi

BUILD_INFO=""
if [[ "${2:-}" =~ ^[a-zA-Z0-9_.-]+$ ]]; then
    BUILD_INFO=".${2,,}"
fi

BUILD_INFO="$(date --utc +'%Y%m%d')$BUILD_INFO"

IMAGE_TAGS=()
if [[ "$GIT_REF" == refs/tags/* ]]; then
    CURRENT_TAG="${GIT_REF:10}"

    IMAGE_TAG="$(echo "$CURRENT_TAG" | sed -e 's/\//-/g')"
    IMAGE_TAGS+=( "$IMAGE_TAG" "$IMAGE_TAG-$BUILD_INFO" )

    if [[ "$CURRENT_TAG" =~ ^v([0-9]+)(\.([0-9]+))?(\.([0-9]+))?(-[a-zA-Z0-9.-]+)?(\+[a-zA-Z0-9.-]+)?$ ]]; then
        if [ -n "${BASH_REMATCH[7]}" ]; then
            IMAGE_TAG="v${BASH_REMATCH[1]}${BASH_REMATCH[2]}${BASH_REMATCH[4]}${BASH_REMATCH[6]}"
            IMAGE_TAGS+=( "$IMAGE_TAG" "$IMAGE_TAG-$BUILD_INFO" )
        fi
        if [ -z "${BASH_REMATCH[6]}" ]; then
            if [ -n "${BASH_REMATCH[5]}" ]; then
                IMAGE_TAG="v${BASH_REMATCH[1]}.${BASH_REMATCH[3]}"
                IMAGE_TAGS+=( "$IMAGE_TAG" "$IMAGE_TAG-$BUILD_INFO" )
            fi
            if [ -n "${BASH_REMATCH[3]}" ]; then
                IMAGE_TAG="v${BASH_REMATCH[1]}"
                IMAGE_TAGS+=( "$IMAGE_TAG" "$IMAGE_TAG-$BUILD_INFO" )
            fi
        fi
    fi

    LATEST_TAG="$(git_ls_tags "v*" | head -n1)"
    if [ "refs/tags/$CURRENT_TAG" == "$LATEST_TAG" ]; then
        IMAGE_TAGS+=( "latest" )
    fi
elif [[ "$GIT_REF" == refs/heads/* ]]; then
    CURRENT_BRANCH="${GIT_REF:11}"

    IMAGE_TAG="dev-$(echo "$CURRENT_BRANCH" | sed 's/[^a-zA-Z0-9._-]\{1,\}/-/g')"
    IMAGE_TAGS+=( "$IMAGE_TAG" "$IMAGE_TAG-$BUILD_INFO" )
elif [[ "$GIT_REF" =~ ^refs/pull/([0-9]+)/merge$ ]]; then
    CURRENT_PR="${BASH_REMATCH[1]}"

    IMAGE_TAG="pr-$CURRENT_PR"
    IMAGE_TAGS+=( "$IMAGE_TAG" "$IMAGE_TAG-$BUILD_INFO" )
elif [ "$(git rev-parse --verify "$GIT_REF" 2> /dev/null)" != "$GIT_REF" ]; then
    echo "Invalid Git reference given: $GIT_REF" >&2
    exit 1
fi

IMAGE_TAG="sha-$(git rev-parse "$GIT_REF")"
IMAGE_TAGS+=( "$IMAGE_TAG" "$IMAGE_TAG-$BUILD_INFO" )

# the first tag always represents the generic name of this particular Git reference
# (i.e. the generic name of this tag, branch, or PR, no matter which commit points to it right now)
echo "${IMAGE_TAGS[@]}"
