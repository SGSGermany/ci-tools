# ci-tools -- helper/git.sh.inc
# Helper functions to better utilize Git in @SGSGermany's build scripts.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

[ -x "$(which git 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: git" >&2; exit 1; }

__quote_git() {
    [ -n "$1" ] && [ "$1" != "." ] && echo "git -C $(quote "$1")" || echo "git"
}

git_latest() {
    local GIT_REPO="$1"
    local VERSION="$2"

    echo + "VERSION=\"\$(git ls-remote --refs $(quote "$GIT_REPO") $(quote "refs/tags/$VERSION")" \
        "| sed … | sort_semver | head -n 1)\"" >&2
    git ls-remote --refs "$GIT_REPO" "refs/tags/$VERSION" \
        | sed -ne 's#^[0-9a-f]*\s*refs/tags/\(.*\)$#\1#p' \
        | sort_semver \
        | head -n 1
}

git_clone() {
    local GIT_REPO="$1"
    local GIT_REF="$2"
    local GIT_COMMIT=""

    local DIR="${3:-.}"
    local DIR_INFO="${4:-$DIR}"

    if [ ! -d "$DIR" ]; then
        echo + "mkdir -p $(quote "$DIR_INFO")" >&2
        mkdir -p "$DIR"
    fi

    echo + "$(__quote_git "$DIR_INFO") init" >&2
    git -C "$DIR" init

    echo + "$(__quote_git "$DIR_INFO") remote add origin $(quote "$GIT_REPO")" >&2
    git -C "$DIR" remote add "origin" "$GIT_REPO"

    if [[ "$GIT_REF" == refs/* ]]; then
        echo + "GIT_COMMIT=\"\$($(__quote_git "$DIR_INFO") ls-remote --refs origin $(quote "$GIT_REF") | tail -n1 | cut -f1)\"" >&2
        GIT_COMMIT="$(git -C "$DIR" ls-remote --refs origin "$GIT_REF" | tail -n1 | cut -f1)"
    elif [ "$(git -C "$DIR" rev-parse --verify "$GIT_REF" 2> /dev/null)" == "$GIT_REF" ]; then
        GIT_COMMIT="$GIT_REF"
    fi

    if [ -z "$GIT_COMMIT" ]; then
        echo "Invalid reference for Git repository '$GIT_REPO': $GIT_REF" >&2
        return 1
    fi

    echo + "$(__quote_git "$DIR_INFO") fetch --depth 1 origin $(quote "$GIT_COMMIT")" >&2
    git -C "$DIR" fetch --depth 1 origin "$GIT_COMMIT"

    echo + "$(__quote_git "$DIR_INFO") checkout --detach $(quote "$GIT_COMMIT")" >&2
    git -C "$DIR" checkout --detach "$GIT_COMMIT"
}

git_ungit() {
    local DIR="${1:-.}"
    local DIR_INFO="${2:-$DIR}"

    echo + "TMP_DIR=\"\$(mktemp -d)\"" >&2
    local TMP_DIR="$(mktemp -d)"

    echo + "mkdir \"\$TMP_DIR/archive\"" >&2
    mkdir "$TMP_DIR/archive"

    echo + "chown --reference $(quote "$DIR_INFO") \"\$TMP_DIR/archive\"" >&2
    chown --reference "$DIR" "$TMP_DIR/archive"

    echo + "chmod --reference $(quote "$DIR_INFO") \"\$TMP_DIR/archive\"" >&2
    chmod --reference "$DIR" "$TMP_DIR/archive"

    echo + "$(__quote_git "$DIR_INFO") archive --format tar HEAD | tar -C \"\$TMP_DIR/archive\" -xf -" >&2
    git -C "$DIR" archive --format "tar" HEAD | tar -C "$TMP_DIR/archive" -xf -

    echo + "rm -rf $(quote "$DIR_INFO")" >&2
    rm -rf "$DIR"

    echo + "mv \"\$TMP_DIR/archive\" $(quote "$DIR_INFO")" >&2
    mv "$TMP_DIR/archive" "$DIR"

    echo + "rmdir \"\$TMP_DIR\"" >&2
    rmdir "$TMP_DIR"
}
