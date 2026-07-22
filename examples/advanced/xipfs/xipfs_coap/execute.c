/*
 * SPDX-FileCopyrightText: 2026 Université de Lille
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "execute.h"

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>

#include "shell.h"
#include "fs/xipfs_fs.h"
#include "utils.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#define STR_HELPER(x) #x

#define STR(x)        STR_HELPER(x)

#define INT_MIN_STR STR(INT_MIN)

static char execute_handler_response_buffer[sizeof(INT_MIN_STR)] = { '\0' };
static char execute_handler_payload_buffer[SHELL_DEFAULT_BUFSIZE] = { '\0'};
static char *execute_handler_args[XIPFS_EXEC_ARGC_MAX] = {NULL};

static const char *parse_execute_params(coap_pkt_t *pdu)
{
    size_t args_count = 0;
    const size_t max_args_count = XIPFS_EXEC_ARGC_MAX - 1;
    const uint16_t str_end = pdu->payload_len;
    uint16_t i = 0, j = 0;

    execute_handler_args[0] = NULL;
    execute_handler_payload_buffer[0] = '\0';

    if (str_end == 0) {
        fprintf(stderr, "parse_execute_params: no payload.\n");
        return NULL;
    }

    if (str_end >= sizeof(execute_handler_payload_buffer)) {
        fprintf(stderr,
                "parse_execute_params: "
                "payload len (%" PRIu16 ") >= buffer size (%zu).\n",
                str_end, sizeof(execute_handler_payload_buffer));
        return NULL;
    }

    memcpy(execute_handler_payload_buffer, pdu->payload, str_end);
    execute_handler_payload_buffer[str_end] = '\0';

    DEBUG("parse_execute_params: execute_handler_payload_buffer : '%s'\n",
           execute_handler_payload_buffer);

    i = find_next_param_separator(execute_handler_payload_buffer, 0, str_end);
    while ( (i < str_end) && (args_count < max_args_count) ) {
        execute_handler_payload_buffer[i] = '\0';
        execute_handler_args[args_count++] =
            &execute_handler_payload_buffer[j];

        j = i + 1;
        i = find_next_param_separator(execute_handler_payload_buffer,
                                      i + 1, str_end);
    }

    execute_handler_args[args_count++] =
            &execute_handler_payload_buffer[j];

    execute_handler_args[args_count] = NULL;

    DEBUG("parse_execute_params: args_count : %zu\n", args_count);
#if (ENABLE_DEBUG == 1)
    for (size_t k = 0; k < args_count; k++) {
        DEBUG("parse_execute_params: arg[%zu] : %s\n",
               k, execute_handler_args[k]);
    }
#endif

    return execute_handler_payload_buffer;
}

typedef int (*xipfs_extended_driver_exec_cb_t)(const char *full_path, char *const argv[]);

static ssize_t execute_base_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                    coap_request_ctx_t *ctx,
                                    xipfs_extended_driver_exec_cb_t exec_cb)
{
    (void)ctx;
    int errorcode = COAP_CODE_INTERNAL_SERVER_ERROR;

    if (coap_get_method(pdu) != COAP_GET) {
        errorcode = COAP_CODE_METHOD_NOT_ALLOWED;
        goto error;
    }

    const char *full_path = parse_execute_params(pdu);
    if (full_path == NULL) {
        errorcode = COAP_CODE_BAD_REQUEST;
error :
        int header_len = coap_build_reply(pdu, errorcode, buf, len, 0);
        if (header_len <= 0) {
            return -1;
        }

        pdu->options_len = 0;
        pdu->payload     = buf + header_len;
        pdu->payload_len = len - header_len;
        return coap_opt_finish(pdu, COAP_OPT_FINISH_NONE);
    }

    int ret = exec_cb(full_path, execute_handler_args);
    ret = snprintf(execute_handler_response_buffer, sizeof(execute_handler_response_buffer), "%d", ret);

    DEBUG("execute_base_handler: execute_handler_response_buffer : '%s', ret %d\n",
           execute_handler_response_buffer, ret);

    coap_block_slicer_t slicer;
    coap_block2_init(pdu, &slicer);

    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    coap_opt_add_block2(pdu, &slicer, 1);
    ssize_t plen = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    if (coap_blockwise_put_bytes_pkt(pdu, &slicer,
                                     execute_handler_response_buffer, ret)) {
        fprintf(stderr,
                "execute_base_handler: failed to put bytes in packet\n");
        return ret;
    }

    coap_block2_finish(&slicer);
    return plen + ret;
}

ssize_t execute_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                        coap_request_ctx_t *ctx)
{
    return execute_base_handler(pdu, buf, len, ctx,
                                xipfs_extended_driver_execv);
}

ssize_t execute_protected_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len,
                                  coap_request_ctx_t *ctx)
{
    return execute_base_handler(pdu, buf, len, ctx,
                                xipfs_extended_driver_safe_execv);
}
