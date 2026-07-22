/*
 * SPDX-FileCopyrightText: 2026 Université de Lille
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "create_exe.h"

#ifdef XIPFS_COAP_CREATE_EXE_ENABLED

#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#include "fs/xipfs_fs.h"
#include "utils.h"

#define ENABLE_DEBUG 0
#include "debug.h"

static const char RESPONSE_MSG[] = "OK";

typedef struct create_exe_params_s {
    const char *filename;
    uint32_t bytesize;
    uint32_t exec_rights;
} create_exe_params_t;

static int parse_create_exe_params(coap_pkt_t *pdu, create_exe_params_t *params)
{
    char *payload = (char *)pdu->payload;
    const uint16_t str_end = pdu->payload_len;
    uint16_t separator_pos = find_next_param_separator(payload, 0, str_end);
    if (separator_pos >= str_end) {
        fprintf(stderr, "parse_create_exe_params: No filename separator found.\n");
        return -1;
    }
    params->filename = payload;
    payload[separator_pos] = '\0';

    const char *bytesize_str = &payload[separator_pos + 1];
    separator_pos = find_next_param_separator(payload, separator_pos + 1, str_end);
    if (separator_pos >= str_end) {
        fprintf(stderr, "parse_create_exe_params: No bytesize separator found.\n");
        return -1;
    }
    payload[separator_pos] = '\0';
    errno = 0;
    long value = strtol(bytesize_str, NULL, 10);
    if (errno != 0) {
        fprintf(stderr, "parse_create_exe_params: invalid bytesize '%s'.\n", bytesize_str);
        return -1;
    }
    if (value < 0) {
        fprintf(stderr, "parse_create_exe_params: invalid negative bytesize '%s'.\n",
                bytesize_str);
        return -1;
    }
    params->bytesize = (uint32_t)value;

    if (payload[separator_pos + 1] == '0')
        params->exec_rights = 0;
    else
        params->exec_rights = 1;

    DEBUG("parse_create_exe_params : '%s', %" PRIu32 ", %" PRIu32 ".\n",
           params->filename, params->bytesize, params->exec_rights);

    return 0;
}

ssize_t create_exe_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                           coap_request_ctx_t *ctx)
{
    uint8_t errorcode = COAP_CODE_INTERNAL_SERVER_ERROR;

    (void)ctx;

    switch (coap_get_method(pdu)) {
        case COAP_METHOD_PUT: {
            create_exe_params_t create_exe_params;
            int ret = parse_create_exe_params(pdu, &create_exe_params);
            if (ret < 0) {
                errorcode = COAP_CODE_BAD_REQUEST;
                goto error;
            }

            ret = xipfs_extended_driver_new_file( create_exe_params.filename,
                                                  create_exe_params.bytesize,
                                                  create_exe_params.exec_rights
                                                 );
            if (ret < 0) {
                printf("create_exe_handler: PUT: failed to create '%s' : error=%d\n",
                        create_exe_params.filename, ret);
                errorcode = COAP_CODE_INTERNAL_SERVER_ERROR;
                goto error;
            }

            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);

            /* Set content format to plain text */
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);

            /* Finalize options and get payload pointer */
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

            if (pdu->payload_len >= sizeof(RESPONSE_MSG)) {
                memcpy(pdu->payload, RESPONSE_MSG, sizeof(RESPONSE_MSG) - 1);
                return resp_len + sizeof(RESPONSE_MSG) - 1;
            }

            fprintf(stderr, "create_exe_handler: gcoap: msg buffer too small\n");
            return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
        }
        default: {
            errorcode = COAP_CODE_METHOD_NOT_ALLOWED;
            goto error;
        }
    }
error:
    int header_len = coap_build_reply(pdu, errorcode, buf, len, 0);

    /* request contained no-response option or not enough space for response */
    if (header_len <= 0) {
        return -1;
    }

    pdu->options_len = 0;
    pdu->payload     = buf + header_len;
    pdu->payload_len = len - header_len;
    return coap_opt_finish(pdu, COAP_OPT_FINISH_NONE);
}

#endif /* XIPFS_COAP_CREATE_EXE_ENABLED */
