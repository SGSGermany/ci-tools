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

[ -x "$(which curl 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: curl" >&2; exit 1; }

[ -x "$(which jq 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: jq" >&2; exit 1; }

github_latest() {
    local GITHUB_REPO_IDENT="$1"

    local CURL_HEADERS=()
    CURL_HEADERS+=( -H "X-GitHub-Api-Version: 2022-11-28" )
    CURL_HEADERS+=( -H "Accept: application/vnd.github+json" )
    [ -z "${GITHUB_TOKEN:-}" ] || CURL_HEADERS=( -H "Authorization: Bearer $GITHUB_TOKEN" )

    echo + "RESPONSE=\"\$(curl -sSL $(quote "https://api.github.com/repos/$GITHUB_REPO_IDENT/releases/latest"))\"" >&2
    local RESPONSE="$(_curl "${CURL_HEADERS[@]}" "https://api.github.com/repos/$GITHUB_REPO_IDENT/releases/latest")"
    [ $? -eq 0 ] || return 1

    echo + "VERSION=\"\$(sed -e '1,/^$/d' <<< \"\$RESPONSE\" | jq '.tag_name')\"" >&2
    sed -e '1,/^$/d' <<< "$RESPONSE" | jq -r '.tag_name'
    [ $? -eq 0 ] || return 1
}
