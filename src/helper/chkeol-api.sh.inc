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

[ -x "$(type -P curl 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: curl" >&2; exit 1; }

[ -x "$(type -P jq 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: jq" >&2; exit 1; }

chkeol_api() {
    local JQ_STATUS_FILTER=".isMaintained | not"
    local JQ_DATE_FILTER=".eolFrom"
    if [ "${1:-}" == "--filter" ]; then
        [ -n "${2:-}" ] || { echo "Missing required parameter 'STATUS' for option '--filter'" >&2; return 1; }
        [ -n "${3:-}" ] || { echo "Missing required parameter 'DATE' for option '--filter'" >&2; return 1; }

        JQ_STATUS_FILTER="$2"
        JQ_DATE_FILTER="$3"
        shift 3
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
    local EOL_STATUS EOL_DATE CURRENT_DATE

    echo + "API_URL=\"https://endoflife.date/api/v1/products/${PROJECT,,}/releases/${CYCLE,,}\"" >&2
    API_URL="https://endoflife.date/api/v1/products/${PROJECT,,}/releases/${CYCLE,,}"

    echo + "API_RESPONSE=\"\$(_curl_json $(quote "$API_URL"))\"" >&2
    API_RESPONSE="$(_curl_json "$API_URL")"

    echo + "CYCLE_INFO=\"\$(jq .result <<< \"\$API_RESPONSE\")\"" >&2
    CYCLE_INFO="$(jq '.result // empty' <<< "$API_RESPONSE")"

    if [ -z "$CYCLE_INFO" ]; then
        echo "Invalid endoflife.date API response: Malformed result" >&2
        return 1
    fi

    echo + "EOL_STATUS=\"\$(jq -r $(quote "$JQ_STATUS_FILTER") <<< \"\$CYCLE_INFO\")\"" >&2
    EOL_STATUS="$(jq -r "$JQ_STATUS_FILTER" <<< "$CYCLE_INFO")"

    if [ "$EOL_STATUS" != "true" ] && [ "$EOL_STATUS" != "false" ]; then
        echo "Invalid endoflife.date API response: Malformed support status: $EOL_STATUS" >&2
        return 1
    fi

    echo + "EOL_DATE=\"\$(jq -r $(quote "$JQ_DATE_FILTER") <<< \"\$CYCLE_INFO\")\"" >&2
    EOL_DATE="$(jq -r "$JQ_DATE_FILTER" <<< "$CYCLE_INFO")"

    if [ "$EOL_DATE" != "null" ] && [[ ! "$EOL_DATE" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
        echo "Invalid endoflife.date API response: Malformed end-of-life date: $EOL_DATE" >&2
        return 1
    fi

    echo + "CURRENT_DATE=\"\$(date +'%Y-%m-%d')\"" >&2
    CURRENT_DATE="$(date +'%Y-%m-%d')"

    if [ "$EOL_STATUS" == "false" ]; then
        if [ "$EOL_DATE" == "null" ]; then
            echo "$PROJECT $CYCLE is still supported"
        elif [[ ! "$CURRENT_DATE" > "$EOL_DATE" ]]; then
            echo "$PROJECT $CYCLE is supported till $EOL_DATE"
        else
            echo "$PROJECT $CYCLE should have reached its end of life on $EOL_DATE, but is still supported"
        fi
    else
        EXIT_CODE=1
        if [ "$EOL_DATE" == "null" ]; then
            echo "$PROJECT $CYCLE has reached its end of life"
        elif [[ "$CURRENT_DATE" > "$EOL_DATE" ]]; then
            echo "$PROJECT $CYCLE has reached its end of life on $EOL_DATE"
        else
            echo "$PROJECT $CYCLE should be supported till $EOL_DATE, but has reached its end of life already"
        fi
    fi

    LATEST_VERSION="$(jq -r '.latest.name // empty' <<< "$CYCLE_INFO")"
    LATEST_VERSION_DATE="$(jq -r '.latest.date // empty' <<< "$CYCLE_INFO")"
    [ -z "$LATEST_VERSION" ] || [ -z "$LATEST_VERSION_DATE" ] || echo "The latest version $LATEST_VERSION was released on $LATEST_VERSION_DATE"
    [ -z "$LATEST_VERSION" ] || [ -n "$LATEST_VERSION_DATE" ] || echo "The latest version is $LATEST_VERSION"
    [ -n "$LATEST_VERSION" ] || [ -z "$LATEST_VERSION_DATE" ] || echo "The latest version was released on $LATEST_VERSION_DATE"

    return $EXIT_CODE
}
