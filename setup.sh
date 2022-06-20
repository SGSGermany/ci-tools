#!/bin/bash
# ci-tools -- setup.sh
# Downloads and setup tools from @SGSGermany's `ci-tools` collection.
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

GITHUB_SERVER_URL="https://github.com"
REPOSITORY="SGSGermany/ci-tools"

GIT_DIR="${1:-}"
[ -n "$GIT_DIR" ] || GIT_DIR="$(mktemp -d)"

GIT_REF="${2:-}"
[ -n "$GIT_REF" ] || GIT_REF="refs/heads/main"

echo "Preparing 'ci-tools' repository..." >&2
[ -d "$GIT_DIR" ] || mkdir "$GIT_DIR"
cd "$GIT_DIR"
git init >&2
git remote add "origin" "$GITHUB_SERVER_URL/$REPOSITORY.git" >&2

echo "Fetching '$GIT_REF' from '$GITHUB_SERVER_URL/$REPOSITORY.git'..." >&2
[[ "$GIT_REF" != refs/* ]] || GIT_REF="$(git ls-remote --refs origin "$GIT_REF" | tail -n1 | cut -f1)"
git fetch --depth 1 origin "$GIT_REF" >&2

echo "Checking out commit $GIT_REF from $(git show -s --format=%ci FETCH_HEAD)..." >&2
git checkout --detach "$GIT_REF" >&2

echo "Setting 'CI_TOOLS' environment variable: SGSGermany" >&2
printf 'export %s="%s"\n' "CI_TOOLS" "SGSGermany"

echo "Setting 'CI_TOOLS_PATH' environment variable: $GIT_DIR/src" >&2
printf 'export %s="%s"\n' "CI_TOOLS_PATH" "$GIT_DIR/src"

if [ "${GITHUB_ACTIONS:-false}" == "true" ]; then
    echo "Switching to containers storage driver 'vfs'..." >&2
    [ -e "${XDG_CONFIG_HOME:-$HOME/.config}/containers" ] \
        || mkdir -p "${XDG_CONFIG_HOME:-$HOME/.config}/containers"
    printf '[storage]\ndriver = "%s"\n' "vfs" \
        > "${XDG_CONFIG_HOME:-$HOME/.config}/containers/storage.conf"
fi
