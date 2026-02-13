# ci-tools -- helper/gpg.sh.inc
# Helper functions to better utilize GnuPG in @SGSGermany's build scripts.
#
# Copyright (c) 2023  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

source "$(dirname "${BASH_SOURCE[0]}")/common-traps.sh.inc"

[ -x "$(type -P gpg 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: gpg" >&2; exit 1; }

gpg_recv() {
    local FILE="$1"
    shift

    if [ -e "$FILE" ]; then
        [ -f "$FILE" ] || { echo "Invalid GnuPG keys file: Not a file" >&2 ; return 1 ; }
        [ -r "$FILE" ] && [ -w "$FILE" ] || { echo "Invalid GnuPG keys file: Permission denied" >&2 ; return 1 ; }
        [ ! -s "$FILE" ] || { echo "Invalid GnuPG keys file: File is not empty" >&2 ; return 1 ; }
    fi

    echo + "GNUPGHOME=\"\$(mktemp -d)\"" >&2
    local GNUPGHOME="$(mktemp -d)"

    trap_exit rm -rf "$GNUPGHOME"

    local KEY
    for KEY in "$@"; do
        cmd gpg --homedir "$GNUPGHOME" --keyserver "keyserver.ubuntu.com" --recv-key "$1" \
            || cmd gpg --homedir "$GNUPGHOME" --keyserver "pgp.mit.edu" --recv-key "$1" \
            || cmd gpg --homedir "$GNUPGHOME" --keyserver "keyserver.pgp.com" --recv-key "$1" \
            || cmd gpg --homedir "$GNUPGHOME" --keyserver "keys.openpgp.org" --recv-key "$1"

        echo + "gpg --homedir $(quote "$GNUPGHOME") --armor --export $(quote "$1") >> $(quote "$FILE")"
        gpg --homedir "$GNUPGHOME" --armor --export "$1" >> "$FILE"
    done
}

gpg_verify() {
    local FILE="$1"
    local SIG_FILE="$2"
    shift 2

    echo + "GNUPGHOME=\"\$(mktemp -d)\"" >&2
    local GNUPGHOME="$(mktemp -d)"

    trap_exit rm -rf "$GNUPGHOME"

    local KEY_FILE
    for KEY_FILE in "$@"; do
        cmd gpg --homedir "$GNUPGHOME" --import "$KEY_FILE"
    done

    cmd gpg --homedir "$GNUPGHOME" --verify "$SIG_FILE" "$FILE"
}
