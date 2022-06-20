#!/bin/bash
# ci-tools -- containers/check-for-updates.sh
# Checks whether a container image must be rebuild or retagged.
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

[ -v CI_TOOLS ] && [ "$CI_TOOLS" == "SGSGermany" ] \
    || { echo "Invalid build environment: Environment variable 'CI_TOOLS' not set or invalid" >&2; exit 1; }

[ -v CI_TOOLS_PATH ] && [ -d "$CI_TOOLS_PATH" ] \
    || { echo "Invalid build environment: Environment variable 'CI_TOOLS_PATH' not set or invalid" >&2; exit 1; }

source "$CI_TOOLS_PATH/helper/common.sh.inc"
source "$CI_TOOLS_PATH/helper/chkupd.sh.inc"

[ -n "${REGISTRY:-}" ] || { echo "Invalid build environment: Missing required env variable 'REGISTRY'" >&2; exit 1; }
[ -n "${OWNER:-}" ] || { echo "Invalid build environment: Missing required env variable 'OWNER'" >&2; exit 1; }
[ -n "${IMAGE:-}" ] || { echo "Invalid build environment: Missing required env variable 'IMAGE'" >&2; exit 1; }
[ -n "${TAGS:-}" ] || { echo "Invalid build environment: Missing required env variable 'TAGS'" >&2; exit 1; }

chkupd_baseimage "$REGISTRY/$OWNER/$IMAGE" "${TAGS%% *}" "${1:-}"
