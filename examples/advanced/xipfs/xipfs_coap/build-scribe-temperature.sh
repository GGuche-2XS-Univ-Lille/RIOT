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

path_to_example='../../../../../riot-xipfs-demonstrations/22-scribe-temperature/'
which_scale=
which_scale_default='TEMPERATURE_SCALE_CELSIUS=1'

usage() {
    name=$(basename "$0")

    printf "\
Usage:
  %s [OPTIONS] <ACTIONS>

  OPTIONS:

    -p|--path-to-example=path   Specify the path to scribe-temperature
    -c|-C|--celsius             Build scribe-temperature with Celsius scale
    -k|-K|--kelvin              Build scribe-temperature with Kelvin scale

  ACTIONS:

    -h|--help                   Display this help

  DEFAULT VALUES:
    - path_to_example=%s
    - which_scale=%s
" "$name" "$path_to_example" "$which_scale_default"
    return 0
}

check_scale_not_set() {
    if [ -n "$which_scale" ] ; then
        echo "Scale is defined more than once in command line."
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
            -p|--path-to-example)
                path_to_example="$value"
                ;;
            -c|-C|--celsius)
                check_scale_not_set
                which_scale='TEMPERATURE_SCALE_CELSIUS=1'
                ;;
            -k|-K|--kelvin)
                check_scale_not_set
                which_scale='TEMPERATURE_SCALE_KELVIN=1'
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
do_build() {
    cd "$path_to_example" && make realclean && make "$1" && cd -
}

main() {
    if [ "$#" -lt 0 ] || [ "$#" -gt 2 ]; then
        usage
        exit 1
    fi

    parse_arguments "$@"

    if [ -z "$which_scale" ] ; then
        echo "Scale is not defined in command line, fallback to --celsius."
        which_scale=$which_scale_default
    fi

    do_build "$which_scale"

    exit 0
}

main "$@"
