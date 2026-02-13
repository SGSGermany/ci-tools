#!/bin/bash
# ci-tools -- containers/setup-github-credentials.sh
# Sets up `git` to pull private GitHub repos using a Personal Access Token.
#
# Copyright (c) 2025  SGS Serious Gaming & Simulations GmbH
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

[ -x "$(type -P git 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: git" >&2; exit 1; }

[ -v GITHUB_TOKEN ] && [ -n "$GITHUB_TOKEN" ] \
    || { echo "Invalid build environment: Environment variable 'GITHUB_TOKEN' not set or invalid" >&2; exit 1; }

cmd git config --global credential.helper "store"
cmd git config --global --add url."https://github.com/".insteadOf "ssh://git@github.com/"
cmd git config --global --add url."https://github.com/".insteadOf "git@github.com:"

echo + "git credential approve < <(printf '%s=%s\n'" \
    "url \"https://github.com/\" username \"git\" password \"\$GITHUB_TOKEN\")" >&2
git credential approve < <(printf '%s=%s\n' \
    "url" "https://github.com/" "username" "git" "password" "$GITHUB_TOKEN")
