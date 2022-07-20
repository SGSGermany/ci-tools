# ci-tools -- helper/patch.sh.inc
# Helper functions to patch source files in @SGSGermany's build scripts.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

patch_apply() {
    local CONTAINER="$1"
    local MOUNT="$(buildah mount "$CONTAINER")"

    local DIR="${3:-.}"
    local DIR_INFO="${4:-$DIR}"

    echo + "rsync -v -rl --exclude .gitignore --exclude '*.patch' $(quote "$DIR_INFO")/ …/" >&2
    rsync -v -rl --exclude '.gitignore' --exclude '*.patch' "$DIR/" "$MOUNT/"

    (
        echo + "find $(quote "$DIR_INFO") -name '*.patch' | while IFS= read -r FILE; do … ; done" >&2

        cd "$DIR"
        find . -name '*.patch' -print0 | while IFS= read -r -d '' FILE; do
            FILE="${FILE:2:-6}"

            echo + "patch -s -u $(quote "…/$FILE") $(quote "$DIR_INFO/$FILE.patch")" >&2
            patch -s -u \
                "$MOUNT/$FILE" \
                "./$FILE.patch"
        done
    )
}
