#!/bin/bash
# ci-tools -- containers/check-end-of-life.sh
# Checks whether a project has reached its EOL using the 'endoflife.date' API.
#
# Copyright (c) 2023  SGS Serious Gaming & Simulations GmbH
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

[ -x "$(which curl 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: curl" >&2; exit 1; }

[ -x "$(which jq 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: jq" >&2; exit 1; }

JQ_FILTER=".eol"
if [ "${1:-}" == "--filter" ]; then
    [ -n "${2:-}" ] || { echo "Missing required parameter 'FILTER' for option '--filter'" >&2; exit 1; }
    JQ_FILTER="$2"
    shift 2
fi

PROJECT="${1:-}"
if [ -z "$PROJECT" ]; then
    echo "Missing required parameter 'PROJECT'" >&2
    exit 1
fi

CYCLE="${2:-}"
if [ -z "$CYCLE" ]; then
    echo "Missing required parameter 'CYCLE'" >&2
    exit 1
fi

EXIT_CODE=0

echo + "API_URL=\"https://endoflife.date/api/${PROJECT,,}/${CYCLE,,}.json\"" >&2
API_URL="https://endoflife.date/api/${PROJECT,,}/${CYCLE,,}.json"

echo + "API_RESPONSE=\"\$(curl -sSL $(quote "$API_URL"))\"" >&2
API_RESPONSE="$(_curl "$API_URL")"
API_RESPONSE="$(sed -e '1,/^$/d' <<< "$API_RESPONSE")"

echo + "CYCLE_INFO=\"\$(jq . <<< \"\$API_RESPONSE\")\"" >&2
CYCLE_INFO="$(jq . <<< "$API_RESPONSE")"

echo + "EOL_DATE=\"\$(jq -r $(quote "$JQ_FILTER") <<< \"\$CYCLE_INFO\")\"" >&2
EOL_DATE="$(jq -r "$JQ_FILTER" <<< "$CYCLE_INFO")"

echo + "[[ $(quote "$EOL_DATE") =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]" >&2
if [[ "$EOL_DATE" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
    echo + "CURRENT_DATE=\"\$(date +'%Y-%m-%d')\"" >&2
    CURRENT_DATE="$(date +'%Y-%m-%d')"

    echo + "[[ $(quote "$CURRENT_DATE") < $(quote "$EOL_DATE") ]]" >&2
    if [[ "$CURRENT_DATE" < "$EOL_DATE" ]]; then
        echo "$PROJECT $CYCLE is supported till $EOL_DATE"
    else
        echo "$PROJECT $CYCLE has reached its end of life on $EOL_DATE"
        EXIT_CODE=1
    fi
else
    echo + "[ $(quote "$EOL_DATE") == false ]" >&2
    if [ "$EOL_DATE" != "false" ]; then
        echo "Invalid endoflife.date API response: Malformed end-of-life date: $EOL_DATE" >&2
        exit 1
    fi

    echo "$PROJECT $CYCLE is still supported"
fi

LATEST_VERSION="$(jq -r '.latest // empty' <<< "$CYCLE_INFO")"
LATEST_VERSION_DATE="$(jq -r '.latestReleaseDate // empty' <<< "$CYCLE_INFO")"
[ -z "$LATEST_VERSION" ] || [ -z "$LATEST_VERSION_DATE" ] \
    || echo "The latest version $LATEST_VERSION was released on $LATEST_VERSION_DATE"

exit $EXIT_CODE
