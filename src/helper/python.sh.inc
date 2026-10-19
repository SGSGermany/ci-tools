# ci-tools -- helper/python.sh.inc
# Helper functions to use Python in @SGSGermany's build scripts.
#
# Copyright (c) 2026  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

python_pip_install() {
    local CONTAINER="$1"

    pkg_install "$CONTAINER" \
        py3-pip

    cmd buildah config \
        --env PYTHONUSERBASE="/usr/local" \
        "$CONTAINER"

    cmd buildah run "$CONTAINER" -- \
        pip config set global.root-user-action ignore

    cmd buildah run "$CONTAINER" -- \
        pip config set global.break-system-packages true
}

python_pip_cleanup() {
    local CONTAINER="$1"

    pkg_remove "$CONTAINER" \
        py3-pip

    echo + "rm -rf …/root/.config/pip …/root/.cache/pip" >&2
    rm -rf "$MOUNT/root/.config/pip" "$MOUNT/root/.cache/pip"
}
