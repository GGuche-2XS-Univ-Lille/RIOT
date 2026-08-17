#!/bin/sh
###############################################################################
#  © Université de Lille, The Pip Development Team (2015-2026)                #
#  Copyright (C) 2020-2025 Orange                                             #
#                                                                             #
#  This software is a computer program whose purpose is to run a minimal,     #
#  hypervisor relying on proven properties such as memory isolation.          #
#                                                                             #
#  This software is governed by the CeCILL license under French law and       #
#  abiding by the rules of distribution of free software.  You can  use,      #
#  modify and/ or redistribute the software under the terms of the CeCILL     #
#  license as circulated by CEA, CNRS and INRIA at the following URL          #
#  "http://www.cecill.info".                                                  #
#                                                                             #
#  As a counterpart to the access to the source code and  rights to copy,     #
#  modify and redistribute granted by the license, users are provided only    #
#  with a limited warranty  and the software's author,  the holder of the     #
#  economic rights,  and the successive licensors  have only  limited         #
#  liability.                                                                 #
#                                                                             #
#  In this respect, the user's attention is drawn to the risks associated     #
#  with loading,  using,  modifying and/or developing or reproducing the      #
#  software by the user in light of its specific status of free software,     #
#  that may mean  that it is complicated to manipulate,  and  that  also      #
#  therefore means  that it is reserved for developers  and  experienced      #
#  professionals having in-depth computer knowledge. Users are therefore      #
#  encouraged to load and test the software's suitability as regards their    #
#  requirements in conditions enabling the security of their systems and/or   #
#  data to be ensured and,  more generally, to use and operate it in the      #
#  same conditions as regards security.                                       #
#                                                                             #
#  The fact that you are presently reading this means that you have had       #
#  knowledge of the CeCILL license and that you accept its terms.             #
###############################################################################

set -e

target_address=
target_address_default='fe80::e416:6ff:fe66:7bf7%tap0'
verbosity=
verbosity_default='3'
path_to_example=
path_to_example_default='../../../../../riot-xipfs-demonstrations/22-scribe-temperature/'
which_scale=
which_scale_default='--celsius'

usage() {
    name=$(basename "$0")

    printf "\
Usage:
  %s [OPTIONS] <ACTIONS>

  OPTIONS:

    -p|--path-to-example=path   Specify the path to scribe-temperature directory (build folder and filename will be deduced automatically)
    -c|-C|--celsius             Build scribe-temperature with Celsius scale
    -k|-K|--kelvin              Build scribe-temperature with Kelvin scale
    -a|--address=ipv6-addr      Specify the target IPV6 address (with tap interface)
    -v|-verbosity=value         Specify coap-client verbosity level (0-9)

  ACTIONS:

    -h|--help                   Display this help

  DEFAULT VALUES:
    - path_to_example=%s
    - target_address=%s
    - verbosity=%s
    - which_scale=%s
" "$name" "$path_to_example_default" "$target_address_default" "$verbosity_default" "$which_scale_default"
    return 0
}

check_path_not_set() {
    if [ -n "$path_to_example" ] ; then
        echo "Path is defined more than once in command line."
        usage && exit 1
    fi
}

check_target_address_not_set() {
    if [ -n "$target_address" ] ; then
        echo "Only one target_address is allowed in command line."
        usage && exit 1
    fi
}

check_verbosity_not_set() {
    if [ -n "$verbosity" ] ; then
        echo "Only one verbosity is allowed in command line."
        usage && exit 1
    fi
}

check_scale_not_set() {
    if [ -n "$which_scale" ] ; then
        echo "Only one scale is allowed in command line."
        usage && exit 1
    fi
}


parse_arguments() {
#    local value
#    local flag

    for argument in "$@"
    do
        value=${argument#*=}
        flag=${argument%=*}
        case "$flag" in
            -h|--help)
                usage && exit 0
                ;;
            -a|--address)
                check_target_address_not_set
                target_address="$value"
                ;;
            -v|-verbosity)
                check_verbosity_not_set
                verbosity="$value"
                ;;
            -p|--path-to-example)
                check_path_not_set
                path_to_example="$value"
                ;;
            -c|-C|--celsius|-k|-K|--kelvin)
                check_scale_not_set
                which_scale="$flag"
                ;;
            *)
                echo "Invalid argument in command line '$flag'."
                usage && exit 1
                ;;
        esac
    done
    return 0
}

# shellcheck disable=SC2329 # code is irrelevant because of indirect invokation in main
# "$path_to_example" "$which_scale" "$target_address" "$verbosity"
do_upload() {
    base_filename=scribe-temperature.fae
    complete_filename="${1%/}"/build/"$base_filename"
    build-scribe-temperature.sh --path-to-example="$1" "$2"
    coap-wrapper.sh -d="$base_filename" -a="$3" -v="$4"
    time -f "%E (real)" coap-wrapper.sh -u="$complete_filename" -a="$3" -v="$4"
}


main() {
    if [ "$#" -lt 0 ] || [ "$#" -gt 4 ]; then
        usage
        exit 1
    fi

    parse_arguments "$@"

    if [ -z "$path_to_example" ] ; then
        path_to_example=$path_to_example_default
        echo "Path is not defined in command line, fallback to $path_to_example."
    fi

    if [ -z "$target_address" ] ; then
        target_address=$target_address_default
        echo "target_address is not defined in command line, fallback to $target_address."
    fi

    if [ -z "$verbosity" ] ; then
        verbosity=$verbosity_default
        echo "verbosity is not defined in command line, fallback to $verbosity."
    fi

    if [ -z "$which_scale" ] ; then
        which_scale=$which_scale_default
        echo "Scale is not defined in command line, fallback to $which_scale."
    fi

    do_upload "$path_to_example" "$which_scale" "$target_address" "$verbosity"

    exit 0
}

main "$@"
