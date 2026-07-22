/*
 * SPDX-FileCopyrightText: 2026 Université de Lille
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef XIPFS_COAP_CREATE_EXE_H
#define XIPFS_COAP_CREATE_EXE_H

#ifdef XIPFS_COAP_CREATE_EXE_ENABLED
#include "net/gcoap.h"

extern ssize_t create_exe_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                  coap_request_ctx_t *ctx);

#endif /* XIPFS_COAP_CREATE_EXE_ENABLED */

#endif /* XIPFS_COAP_CREATE_EXE_H */
