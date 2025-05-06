# ci-tools -- helper/chkeol-api.sh.inc
# Helper functions to check whether a version is still actively supported.
#
# Copyright (c) 2024  SGS Serious Gaming & Simulations GmbH
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

chkeol_api() {
    local JQ_FILTER=".eolFrom"
    if [ "${1:-}" == "--filter" ]; then
        if [ -z "${2:-}" ]; then
            echo "Missing required parameter 'FILTER' for option '--filter'" >&2
            return 1
        fi

        JQ_FILTER="$2"
        shift 2
    fi

    local PROJECT="${1:-}"
    if [ -z "$PROJECT" ]; then
        echo "Missing required argument 'PROJECT'" >&2
        return 1
    fi

    local CYCLE="${2:-}"
    if [ -z "$CYCLE" ]; then
        echo "Missing required argument 'CYCLE'" >&2
        return 1
    fi

    local EXIT_CODE=0
    local API_URL API_RESPONSE
    local CYCLE_INFO
    local EOL_DATE

    echo + "API_URL=\"https://endoflife.date/api/v1/products/${PROJECT,,}/releases/${CYCLE,,}\"" >&2
    API_URL="https://endoflife.date/api/v1/products/${PROJECT,,}/releases/${CYCLE,,}"

    echo + "API_RESPONSE=\"\$(curl -sSL $(quote "$API_URL"))\"" >&2
    API_RESPONSE="$(_curl "$API_URL")"
    API_RESPONSE="$(sed -e '1,/^$/d' <<< "$API_RESPONSE")"

    echo + "CYCLE_INFO=\"\$(jq .result <<< \"\$API_RESPONSE\")\"" >&2
    CYCLE_INFO="$(jq .result <<< "$API_RESPONSE")"

    echo + "EOL_DATE=\"\$(jq -r $(quote "$JQ_FILTER") <<< \"\$CYCLE_INFO\")\"" >&2
    EOL_DATE="$(jq -r "$JQ_FILTER" <<< "$CYCLE_INFO")"

    echo + "[[ $(quote "$EOL_DATE") =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]" >&2
    if [[ "$EOL_DATE" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
        echo + "CURRENT_DATE=\"\$(date +'%Y-%m-%d')\"" >&2
        local CURRENT_DATE="$(date +'%Y-%m-%d')"

        echo + "[[ ! $(quote "$CURRENT_DATE") > $(quote "$EOL_DATE") ]]" >&2
        if [[ ! "$CURRENT_DATE" > "$EOL_DATE" ]]; then
            echo "$PROJECT $CYCLE is supported till $EOL_DATE"
        else
            echo "$PROJECT $CYCLE has reached its end of life on $EOL_DATE"
            EXIT_CODE=1
        fi
    else
        echo + "[ $(quote "$EOL_DATE") == false ]" >&2
        if [ "$EOL_DATE" != "false" ]; then
            echo "Invalid endoflife.date API response: Malformed end-of-life date: $EOL_DATE" >&2
            return 1
        fi

        echo "$PROJECT $CYCLE is still supported"
    fi

    LATEST_VERSION="$(jq -r '.latest.name // empty' <<< "$CYCLE_INFO")"
    LATEST_VERSION_DATE="$(jq -r '.latest.date // empty' <<< "$CYCLE_INFO")"
    [ -z "$LATEST_VERSION" ] || [ -z "$LATEST_VERSION_DATE" ] || echo "The latest version $LATEST_VERSION was released on $LATEST_VERSION_DATE"
    [ -z "$LATEST_VERSION" ] || [ -n "$LATEST_VERSION_DATE" ] || echo "The latest version is $LATEST_VERSION"
    [ -n "$LATEST_VERSION" ] || [ -z "$LATEST_VERSION_DATE" ] || echo "The latest version was released on $LATEST_VERSION_DATE"

    return $EXIT_CODE
}
