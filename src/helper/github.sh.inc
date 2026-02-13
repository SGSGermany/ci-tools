# ci-tools -- helper/github.sh.inc
# Helper functions to work with GitHub's API in @SGSGermany's build scripts.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

[ -x "$(type -P curl 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: curl" >&2; exit 1; }

[ -x "$(type -P jq 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: jq" >&2; exit 1; }

github_latest() {
    local GITHUB_REPO_IDENT="$1"

    local RESPONSE CURL_HEADERS=()
    CURL_HEADERS+=( -H "X-GitHub-Api-Version: 2022-11-28" )
    CURL_HEADERS+=( -H "Accept: application/vnd.github+json" )
    [ -z "${GITHUB_TOKEN:-}" ] || CURL_HEADERS=( -H "Authorization: Bearer $GITHUB_TOKEN" )

    echo + "RESPONSE=\"\$(_curl_json $(quote "https://api.github.com/repos/$GITHUB_REPO_IDENT/releases/latest"))\"" >&2
    RESPONSE="$(_curl_json "${CURL_HEADERS[@]}" "https://api.github.com/repos/$GITHUB_REPO_IDENT/releases/latest")"

    echo + "jq -er '.tag_name' <<< \"\$RESPONSE\"" >&2
    jq -er '.tag_name' <<< "$RESPONSE"
}
