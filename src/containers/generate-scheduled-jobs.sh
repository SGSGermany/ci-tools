#!/bin/bash
# ci-tools -- containers/generate-scheduled-jobs.sh
# Generates a list of scheduled jobs to build @SGSGermany container images.
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

git_ls_branches() {
    git ls-remote "$GIT_REMOTE_URL" "refs/heads/$1" | cut -f 2
}

git_ls_tags() {
    git ls-remote "$GIT_REMOTE_URL" "refs/tags/$1" | cut -f 2 | sort_semver
}

sort_semver() {
    sed '/-/!{s/$/_/}' | sort -V -r "$@" | sed 's/_$//'
}

if [ -z "$GIT_REMOTE_URL" ]; then
    echo "Invalid build environment: Missing required env variable 'GIT_REMOTE_URL'" >&2
    exit 1
fi

# run on branches
BRANCHES=()
for BRANCH_PATTERN in $RUN_ON_BRANCHES; do
    while IFS= read -r BRANCH; do
        BRANCHES+=( "$BRANCH" )
    done < <(git_ls_branches "$BRANCH_PATTERN")
done

IGNORE_BRANCHES=()
for BRANCH_PATTERN in $RUN_ON_BRANCHES_IGNORE; do
    while IFS= read -r BRANCH; do
        IGNORE_BRANCHES+=( "$BRANCH" )
    done < <(git_ls_branches "$BRANCH_PATTERN")
done

if [ ${#BRANCHES[@]} -gt 0 ]; then
    if [ ${#IGNORE_BRANCHES[@]} -gt 0 ]; then
        {
            printf '%s\n' "${BRANCHES[@]}" | sort -u;
            printf '%s\n' "${IGNORE_BRANCHES[@]}" "${IGNORE_BRANCHES[@]}";
        } | sort | uniq -u
    else
        printf '%s\n' "${BRANCHES[@]}" | sort -u
    fi
fi

# run on tags
TAGS=()
for TAG_PATTERN in $RUN_ON_TAGS; do
    while IFS= read -r TAG; do
        TAGS+=( "$TAG" )
    done < <(git_ls_tags "$TAG_PATTERN")
done

for TAG_PATTERN in $RUN_ON_TAGS_LATEST; do
    TAG="$(git_ls_tags "$TAG_PATTERN" | head -n 1)"
    [ -z "$TAG" ] || TAGS+=( "$TAG" )
done

IGNORE_TAGS=()
for TAG_PATTERN in $RUN_ON_TAGS_IGNORE; do
    while IFS= read -r TAG; do
        IGNORE_TAGS+=( "$TAG" )
    done < <(git_ls_tags "$GIT_REMOTE_URL" "$TAG_PATTERN")
done

if [ ${#TAGS[@]} -gt 0 ]; then
    if [ ${#IGNORE_TAGS[@]} -gt 0 ]; then
        {
            printf '%s\n' "${TAGS[@]}" | sort -u;
            printf '%s\n' "${IGNORE_TAGS[@]}" "${IGNORE_TAGS[@]}";
        } | sort_semver | uniq -u
    else
        printf '%s\n' "${TAGS[@]}" | sort_semver -u
    fi
fi
