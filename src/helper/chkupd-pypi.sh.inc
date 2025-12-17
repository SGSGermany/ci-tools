# ci-tools -- helper/chkupd-pypi.sh.inc
# Helper functions to check whether PyPi package upgrades are available.
#
# Copyright (c) 2023  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

source "$(dirname "${BASH_SOURCE[0]}")/common-traps.sh.inc"

[ -x "$(which jq 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: jq" >&2; exit 1; }

chkupd_pypi() {
    local IMAGE="$1"
    shift

    # pull current image
    echo + "CONTAINER=\"\$(buildah from $(quote "$IMAGE"))\"" >&2
    local CONTAINER="$(buildah from "$IMAGE" || true)"

    if [ -z "$CONTAINER" ]; then
        echo "Failed to pull image '$IMAGE': No image with this tag found" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    trap_exit buildah rm "$CONTAINER"

    # check whether image is based on Alpine Linux
    echo + "if ! buildah run --user root $(quote "$CONTAINER") -- test -f /etc/alpine-release; then … fi" >&2
    if ! buildah run --user root "$CONTAINER" -- test -f "/etc/alpine-release"; then
        echo "Invalid image '$IMAGE': Expecting an Alpine Linux image" >&2
        echo "Internal error" >&2
        echo "build"
        return 2
    fi

    # install pip
    cmd buildah run --user root "$CONTAINER" -- \
        apk add --no-cache py3-pip@community >&2

    # check for PyPi package upgrades
    echo + "PACKAGE_UPGRADES=\"\$(buildah run --user root $(quote "$CONTAINER") -- pip list --user --outdated)\"" >&2
    local PACKAGE_UPGRADES="$(buildah run --user root "$CONTAINER" -- pip list --user --outdated)"

    if [ -n "$PACKAGE_UPGRADES" ]; then
        echo "Image is out of date: PyPi package upgrades are available" >&2
        echo "$PACKAGE_UPGRADES" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi
}

chkupd_pypi_dependencies() {
    local IMAGE="$1"
    shift

    # pull current image
    echo + "CONTAINER=\"\$(buildah from $(quote "$IMAGE"))\"" >&2
    local CONTAINER="$(buildah from "$IMAGE" || true)"

    if [ -z "$CONTAINER" ]; then
        echo "Failed to pull image '$IMAGE': No image with this tag found" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    trap_exit buildah rm "$CONTAINER"

    # mount current image
    echo + "MOUNT=\"\$(buildah mount $(quote "$CONTAINER"))\"" >&2
    local MOUNT="$(buildah mount "$CONTAINER" || true)"

    if [ -z "$CONTAINER" ]; then
        echo "Invalid image '$IMAGE': Failed to mount image" >&2
        echo "Internal error" >&2
        echo "build"
        return 2
    fi

    # check whether image is based on Alpine Linux
    echo + "[ -f …/etc/alpine-release ]" >&2
    if [ ! -f "$MOUNT/etc/alpine-release" ]; then
        echo "Invalid image '$IMAGE': Expecting an Alpine Linux image" >&2
        echo "Internal error" >&2
        echo "build"
        return 2
    fi

    # install pip and jq
    cmd buildah run --user root "$CONTAINER" -- \
        apk add --no-cache py3-pip@community jq >&2

    # get requirements of requested packages
    local PACKAGES=() PACKAGE METADATA DEPENDENCIES=() DEPENDENCY
    for PACKAGE in "$@"; do
        # get metadata file of package
        echo + "METADATA=\"\$(buildah run --user root $(quote "$CONTAINER") -- pip show $(quote "$PACKAGE")" \
            "| awk -F': ' '/^Name: / {n=\$2} /^Version: / {v=\$2} /^Location: / {l=\$2} END {print l \"/\" n \"-\" v \".dist-info/METADATA\"}')\"" >&2
        METADATA="$(buildah run --user root "$CONTAINER" -- pip show "$PACKAGE" \
            | awk -F': ' '/^Name: / {n=$2} /^Version: / {v=$2} /^Location: / {l=$2} END {print l "/" n "-" v ".dist-info/METADATA"}')"

        # get dependency list of package (without version constraints)
        echo + "readarray -t DEPENDENCIES < <(buildah run --user root $(quote "$CONTAINER") -- pip show $(quote "$PACKAGE")" \
            "| sed -ne 's#^Requires: \(.*\)$#\1#p' | sed -e 's#,\s*#\n#g' )" >&2
        readarray -t DEPENDENCIES < <(buildah run --user root "$CONTAINER" -- pip show "$PACKAGE" \
             | sed -ne 's#^Requires: \(.*\)$#\1#p' | sed -e 's#,\s*#\n#g')

        # read version constraints from metadata file and add everything to package list
        echo + "for DEPENDENCY in \"\${DEPENDENCIES[@]}\"; do PACKAGES+=( \"\$(awk -v pkg=\"\$DEPENDENCY\"" \
            "-F': ' '\$0 ~ \"^Requires-Dist: \" pkg {print \$2}' $(quote "…/$METADATA"))\" ); done" >&2
        for DEPENDENCY in "${DEPENDENCIES[@]}"; do
            PACKAGES+=( "$(awk -v pkg="$DEPENDENCY" -F': ' '$0 ~ "^Requires-Dist: " pkg {print $2}' "$MOUNT/$METADATA")" )
        done
    done

    # check for PyPi package upgrades
    echo + "readarray -t PACKAGE_UPGRADES < <( \\" >&2
    echo + "    buildah run --user root $(quote "$CONTAINER") -- \\" >&2
    echo + "        pip install --user --upgrade --dry-run --quiet $(quote "${PACKAGES[@]}") --report - 2> /dev/null \\" >&2
    echo + "            | jq -r '.install[] | \"\(.metadata.name)  \(.metadata.version)\"' \\" >&2
    echo + ")" >&2

    local PACKAGE_UPGRADES=()
    readarray -t PACKAGE_UPGRADES < <( \
        buildah run --user root "$CONTAINER" -- \
            pip install --user --upgrade --dry-run --quiet "${PACKAGES[@]}" --report - 2> /dev/null \
                | jq -r '.install[] | "\(.metadata.name)  \(.metadata.version)"' \
    )

    if [ ${#PACKAGE_UPGRADES[@]} -gt 0 ]; then
        echo "Image is out of date: PyPi package upgrades are available" >&2
        printf '%s\n' "${PACKAGE_UPGRADES[@]}" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi
}

chkupd_pypi_requirements() {
    local IMAGE="$1"
    shift

    # pull current image
    echo + "CONTAINER=\"\$(buildah from $(quote "$IMAGE"))\"" >&2
    local CONTAINER="$(buildah from "$IMAGE" || true)"

    if [ -z "$CONTAINER" ]; then
        echo "Failed to pull image '$IMAGE': No image with this tag found" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi

    trap_exit buildah rm "$CONTAINER"

    # check whether image is based on Alpine Linux
    echo + "if ! buildah run --user root $(quote "$CONTAINER") -- test -f /etc/alpine-release; then … fi" >&2
    if ! buildah run --user root "$CONTAINER" -- test -f "/etc/alpine-release"; then
        echo "Invalid image '$IMAGE': Expecting an Alpine Linux image" >&2
        echo "Internal error" >&2
        echo "build"
        return 2
    fi

    # install pip and jq
    cmd buildah run --user root "$CONTAINER" -- \
        apk add --no-cache py3-pip@community jq >&2

    # check for PyPi package upgrades based on requirements.txt files
    echo + "PACKAGE_UPGRADES=()" >&2
    echo + "for REQUIREMENTS_FILE in \"\$@\"; do" >&2
    echo + "    readarray -t -O \${#PACKAGE_UPGRADES[@]} PACKAGE_UPGRADES < <( \\" >&2
    echo + "        buildah run --user root $(quote "$CONTAINER") -- \\" >&2
    echo + "            pip install --upgrade -r \"\$REQUIREMENTS_FILE\" --dry-run --quiet --report - 2> /dev/null \\" >&2
    echo + "                | jq -r '.install[] | \"\(.metadata.name)  \(.metadata.version)\"' \\" >&2
    echo + "    )" >&2
    echo + "done" >&2

    local PACKAGE_UPGRADES=()
    for REQUIREMENTS_FILE in "$@"; do
        readarray -t -O "${#PACKAGE_UPGRADES[@]}" PACKAGE_UPGRADES < <( \
            buildah run --user root "$CONTAINER" -- \
                pip install --upgrade -r "$REQUIREMENTS_FILE" --dry-run --quiet --report - 2> /dev/null \
                    | jq -r '.install[] | "\(.metadata.name)  \(.metadata.version)"' \
        )
    done

    if [ ${#PACKAGE_UPGRADES[@]} -gt 0 ]; then
        echo "Image is out of date: PyPi package upgrades are available" >&2
        printf '%s\n' "${PACKAGE_UPGRADES[@]}" >&2
        echo "Image rebuild required" >&2
        echo "build"
        return 1
    fi
}
