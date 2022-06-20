# ci-tools -- helper/common-traps.sh.inc
# Helper functions to use exit traps in @SGSGermany's build scripts.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

[ -v TRAPS_EXIT ] || TRAPS_EXIT=()

function trap_exit {
    TRAPS_EXIT+=( "$(quote "$@")" )
}

function trap_exit_exec {
    EXIT=$?

    for (( INDEX=${#TRAPS_EXIT[@]}-1 ; INDEX >= 0 ; INDEX-- )); do
        echo + "${TRAPS_EXIT[$INDEX]}" >&2
        eval "${TRAPS_EXIT[$INDEX]}" >&2
    done

    trap - ERR EXIT
    exit $EXIT
}

trap 'trap_exit_exec' ERR EXIT
