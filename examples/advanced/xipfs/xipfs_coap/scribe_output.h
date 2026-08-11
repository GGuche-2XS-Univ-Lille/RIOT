/*
 * SPDX-FileCopyrightText: 2026 Université de Lille
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef XIPFS_COAP_SCRIBE_OUTPUT_H
#define XIPFS_COAP_SCRIBE_OUTPUT_H

#include "net/gcoap.h"

extern int scribe_output_initialize(int argc, const char *argv[]);
extern ssize_t scribe_output_prepare(coap_pkt_t *pdu, uint8_t *buf, size_t bytesize);
extern ssize_t scribe_output_commit(int exe_return_value);
extern void scribe_output_release(void);


#endif /* XIPFS_COAP_SCRIBE_OUTPUT_H */
