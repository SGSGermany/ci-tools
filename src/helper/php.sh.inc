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

php_install_ext() {
    local CONTAINER="$1"
    shift

    [ $# -gt 0 ] || return 0

    local PACKAGES=()
    local EXT_CONFIGURES=()
    local EXT_INSTALLS=()

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

            "intl")
                PACKAGES+=( "icu-dev" )
                EXT_CONFIGURES+=( "intl" )
                EXT_INSTALLS+=( "intl" )
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

    for EXT_CONFIGURE in "${EXT_CONFIGURES[@]}"; do
        cmd buildah run "$CONTAINER" -- \
            docker-php-ext-configure \
                $EXT_CONFIGURE
    done

    if [ ${#EXT_INSTALLS[@]} -gt 0 ]; then
        cmd buildah run "$CONTAINER" -- \
            docker-php-ext-install -j "$(nproc)" \
                "${EXT_INSTALLS[@]}"
    fi

    if [ ${#PACKAGES[@]} -gt 0 ]; then
        echo + "PHP_RUN_DEPS=( \$(buildah run $(quote "$CONTAINER") scanelf --needed --recursive /usr/local/lib/php/extensions …) )" >&2
        local PHP_RUN_DEPS=( $(
            buildah run "$CONTAINER" -- \
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
}
