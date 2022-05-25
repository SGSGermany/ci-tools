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

set -e
export LC_ALL=C

GITHUB_SERVER_URL="https://github.com"
REPOSITORY="SGSGermany/ci-tools"

GIT_DIR="$1"
if [ -z "$GIT_DIR" ]; then
    GIT_DIR="$(mktemp -d)"
fi

echo "Cloning 'ci-tools' repository from '$GITHUB_SERVER_URL/$REPOSITORY.git'..." >&2
git clone --depth=1 "$GITHUB_SERVER_URL/$REPOSITORY.git" "$GIT_DIR" >&2

COMMIT_SHA="$(git -C "$GIT_DIR" show -s --format=%h HEAD)"
COMMIT_DATE="$(git -C "$GIT_DIR" show -s --format=%ci HEAD)"
echo "Using CI tools as of commit $COMMIT_SHA from $COMMIT_DATE" >&2

echo "Setting 'CI_TOOLS_PATH' environment variable: $GIT_DIR/src" >&2
printf 'export %s="%s"' "CI_TOOLS_PATH" "$GIT_DIR/src"

if [ "${GITHUB_ACTIONS:-false}" == "true" ]; then
    echo "Switching to containers storage driver 'vfs'..." >&2
    [ -e "${XDG_CONFIG_HOME:-$HOME/.config}/containers" ] \
        || mkdir -p "${XDG_CONFIG_HOME:-$HOME/.config}/containers"
    printf '[storage]\ndriver = "%s"\n' "vfs" \
        > "${XDG_CONFIG_HOME:-$HOME/.config}/containers/storage.conf"
fi
