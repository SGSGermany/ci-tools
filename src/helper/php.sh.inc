# ci-tools -- helper/php.sh.inc
# Helper functions to build @SGSGermany's php-fpm containers.
#
# Copyright (c) 2022  SGS Serious Gaming & Simulations GmbH
#
# This work is licensed under the terms of the MIT license.
# For a copy, see LICENSE file or <https://opensource.org/licenses/MIT>.
#
# SPDX-License-Identifier: MIT
# License-Filename: LICENSE

source "$(dirname "${BASH_SOURCE[0]}")/common-traps.sh.inc"
source "$(dirname "${BASH_SOURCE[0]}")/gpg.sh.inc"

[ -x "$(which curl 2>/dev/null)" ] \
    || { echo "Missing CI tools script dependency: curl" >&2; exit 1; }

__COMPOSER_PHAR="https://getcomposer.org/download/%s/composer.phar"
__COMPOSER_PHAR_ASC="https://getcomposer.org/download/%s/composer.phar.asc"
__COMPOSER_GPG_KEYS=( "161DFBE342889F01DDAC4E61CBB3D576F2A0946F" )

php_install_ext() {
    local CONTAINER="$1"
    local MOUNT="$(buildah mount "$CONTAINER")"
    shift

    [ $# -gt 0 ] || return 0

    local PACKAGES=()
    local EXT_CONFIGURES=()
    local EXT_INSTALLS=()
    local EXT_ENABLE=()
    local EXT_DISABLE=()
    local PECL_INSTALLS=()

    local PHPIZE_DEPS=()

    while [ $# -gt 0 ]; do
        case "$1" in
            "bcmath"|"pdo_mysql")
                EXT_INSTALLS+=( "$1" )
                ;;

            "gettext")
                set -- "$1" "intl" "${@:2}"

                PACKAGES+=( "gettext-dev" )
                EXT_CONFIGURES+=( "gettext" )
                EXT_INSTALLS+=( "gettext" )
                ;;

            "gd")
                PACKAGES+=( "freetype-dev" "libjpeg-turbo-dev" "libpng-dev" "libwebp-dev" )
                EXT_CONFIGURES+=( "gd --with-freetype --with-jpeg --with-webp" )
                EXT_INSTALLS+=( "gd" )
                ;;

            "imap")
                PACKAGES+=( "imap-dev" "openssl-dev" )
                EXT_CONFIGURES+=( "imap --with-imap-ssl" )
                EXT_INSTALLS+=( "imap" )
                ;;

            "intl")
                PACKAGES+=( "icu-dev" )
                EXT_CONFIGURES+=( "intl" )
                EXT_INSTALLS+=( "intl" )
                ;;

            "ldap")
                PACKAGES+=( "openldap-dev" )
                EXT_INSTALLS+=( "ldap" )
                ;;

            "xdebug")
                PACKAGES+=( "linux-headers" )
                PECL_INSTALLS+=( "xdebug" )
                EXT_DISABLE+=( "docker-php-ext-xdebug.ini" )
                ;;

            "zip")
                PACKAGES+=( "libzip-dev" )
                EXT_INSTALLS+=( "zip" )
                ;;

            *)
                echo "Unable to install PHP extension '$1': Unknown extension" >&2
                return 1
                ;;
        esac
        shift
    done

    if [ ${#PACKAGES[@]} -gt 0 ]; then
        pkg_install "$CONTAINER" \
            --virtual .php-build-deps \
            "${PACKAGES[@]}"
    fi

    if [ ${#PECL_INSTALLS[@]} -gt 0 ]; then
        echo + "PHPIZE_DEPS=( \$(buildah run --user root $(quote "$CONTAINER") -- sh -c 'echo \"\$PHPIZE_DEPS\"') )" >&2
        PHPIZE_DEPS=( $(buildah run --user root "$CONTAINER" -- sh -c 'echo "$PHPIZE_DEPS"') )

        pkg_install "$CONTAINER" \
            --virtual .phpize-deps \
            "${PHPIZE_DEPS[@]}"

        for PECL_INSTALL in "${PECL_INSTALLS[@]}"; do
            cmd buildah run --user root "$CONTAINER" -- \
                pecl install "$PECL_INSTALL"
            EXT_ENABLE+=( "$PECL_INSTALL" )
        done
    fi

    for EXT_CONFIGURE in "${EXT_CONFIGURES[@]}"; do
        cmd buildah run --user root "$CONTAINER" -- \
            docker-php-ext-configure \
                $EXT_CONFIGURE
    done

    if [ ${#EXT_INSTALLS[@]} -gt 0 ]; then
        cmd buildah run --user root "$CONTAINER" -- \
            docker-php-ext-install -j "$(nproc)" \
                "${EXT_INSTALLS[@]}"
    fi

    if [ ${#EXT_ENABLE[@]} -gt 0 ]; then
        cmd buildah run --user root "$CONTAINER" -- \
            docker-php-ext-enable \
                "${EXT_ENABLE[@]}"
    fi

    if [ ${#EXT_DISABLE[@]} -gt 0 ]; then
        for EXT in "${EXT_DISABLE[@]}"; do
            echo + "[ -f $(quote "…/etc/php/conf.d/$EXT") ]" >&2
            [ -f "$MOUNT/etc/php/conf.d/$EXT" ] || return 1

            echo + "awk $(quote '!/^(\s*|;.*)$/ { print ";" $0 } /^(\s*|;.*)$/ { print }' "…/etc/php/conf.d/$EXT")" \
                "| sponge $(quote "…/etc/php/conf.d/$EXT")" >&2
            awk '!/^(\s*|;.*)$/ { print ";" $0 } /^(\s*|;.*)$/ { print }' "$MOUNT/etc/php/conf.d/$EXT" \
                | sponge "$MOUNT/etc/php/conf.d/$EXT"
        done
    fi

    if [ ${#PACKAGES[@]} -gt 0 ]; then
        echo + "PHP_RUN_DEPS=( \$(buildah run --user root $(quote "$CONTAINER") -- scanelf --needed --recursive /usr/local/lib/php/extensions …) )" >&2
        local PHP_RUN_DEPS=( $(
            buildah run --user root "$CONTAINER" -- \
                scanelf --needed --recursive "/usr/local/lib/php/extensions" --nobanner --format '%n#p' \
                    | tr ',' '\n' \
                    | sort -u \
                    | awk 'system("[ -e /usr/local/lib/" $1 " ]") == 0 { next } { print "so:" $1 }'
        ) )

        if [ ${#PHP_RUN_DEPS[@]} -gt 0 ]; then
            pkg_install "$CONTAINER" \
                --virtual .php-run-deps \
                "${PHP_RUN_DEPS[@]}"
        fi

        pkg_remove "$CONTAINER" \
            .php-build-deps
    fi

    if [ ${#PHPIZE_DEPS[@]} -gt 0 ]; then
        pkg_remove "$CONTAINER" \
            .phpize-deps
    fi
}

php_composer_install() {
    local CONTAINER="$1"
    local COMPOSER_PHAR="$(printf "$__COMPOSER_PHAR" "${2:-latest-stable}")"
    local COMPOSER_PHAR_ASC="$(printf "$__COMPOSER_PHAR_ASC" "${2:-latest-stable}")"
    local TARGET="${3:-/usr/local/bin/composer}"

    echo + "COMPOSER_TMP=\"\$(mktemp -d)\"" >&2
    local COMPOSER_TMP="$(mktemp -d)"

    trap_exit rm -rf "$COMPOSER_TMP"

    cmd curl -L -f -o "$COMPOSER_TMP/composer.phar" "$COMPOSER_PHAR"
    cmd curl -L -f -o "$COMPOSER_TMP/composer.phar.asc" "$COMPOSER_PHAR_ASC"

    gpg_recv "$COMPOSER_TMP/composer.gpg" "${__COMPOSER_GPG_KEYS[@]}"

    gpg_verify "$COMPOSER_TMP/composer.phar" "$COMPOSER_TMP/composer.phar.asc" "$COMPOSER_TMP/composer.gpg"

    echo + "cp $(quote "$COMPOSER_TMP/composer.phar") …$TARGET" >&2
    cp "$COMPOSER_TMP/composer.phar" "$(buildah mount "$CONTAINER")$TARGET"

    cmd buildah run --user root "$CONTAINER" -- \
        chmod +x "$TARGET"
}

php_composer_remove() {
    local TARGET="${2:-/usr/local/bin/composer}"

    echo + "rm -f …$TARGET"
    rm -f "$(buildah mount "$1")$TARGET"
}

php_patch_config() {
    local FILE="$(buildah mount "$1")/$2"
    local CONTENT="$(< "$FILE")"
    shift 2

    while (( $# >= 2 )); do
        CONTENT="$(printf '%s\n' "$CONTENT" | awk -v key="$1" -v value="$2" '
            $0 ~ "^(;[ \t]*)?" key "[ \t]*=" { mlc = NR; found = 1 }
            mlc != 0 && mlc != NR { print key " = " value; mlc = 0 }
            mlc == 0 { print }
            END { if (mlc != 0 || found == 0) { print key " = " value } }
        ' -)"
        shift 2
    done

    [ -n "$CONTENT" ] && printf '%s\n' "$CONTENT" > "$FILE" || : > "$FILE"
}

php_patch_config_list() {
    local FILE="$(buildah mount "$1")/$2"
    shift 2

    awk -v key="$1" -v lines="$(printf "%s\n" "${@:2}")" '
        $0 ~ "^(;[ \t]*)?" key "\\[" { mlc = NR; found = 1 }
        mlc != 0 && mlc != NR { print lines; mlc = 0 }
        mlc == 0 { print }
        END { if (mlc != 0 || found == 0) { print lines } }
    ' "$FILE" | sponge "$FILE"
}
