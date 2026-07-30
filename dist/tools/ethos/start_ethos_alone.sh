#!/bin/sh

ETHOS_DIR="$(dirname $(readlink -f $0))"

create_tap() {
    ip tuntap add ${TAP} mode tap user ${USER}
    sysctl -w net.ipv6.conf.${TAP}.forwarding=1
    sysctl -w net.ipv6.conf.${TAP}.accept_ra=0
    ip link set ${TAP} up
    ip a a fe80::1/64 dev ${TAP}
    ip a a fd00:dead:beef::1/128 dev lo
}

remove_tap() {
    ip tuntap del ${TAP} mode tap
}

cleanup() {
    echo "Cleaning up..."
    remove_tap
    ip a d fd00:dead:beef::1/128 dev lo
    if [ -n "${UHCPD_PID}" ]; then
        kill ${UHCPD_PID}
    fi
    if [ -n "${DHCPD_PIDFILE}" ]; then
        kill "$(cat ${DHCPD_PIDFILE})"
        rm "${DHCPD_PIDFILE}"
    fi
    trap "" INT QUIT TERM EXIT
}

echo DEBUG< $0 $1 $2

PORT=$1
TAP=$2
BAUDRATE=115200

[ -z "${PORT}" -o -z "${TAP}" ] && {
    echo "usage: $0 <serial-port> <tap-device> [baudrate]"
    exit 1
}

[ ! -z $3 ] && {
    BAUDRATE=$3
}

trap "cleanup" INT QUIT TERM EXIT

create_tap && "${ETHOS_DIR}/ethos" ${TAP} ${PORT} ${BAUDRATE}
