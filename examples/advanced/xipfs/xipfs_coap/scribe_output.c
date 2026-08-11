#include "scribe_output.h"

#ifndef SCRIBE_COAP_SINK_ENABLED
#error "COAP support in Scribe not enabled"
#endif

#include <limits.h>

#include "mutex.h"

#include "include/scribe.h"
#include "include/stdout_sink.h"
#include "include/coap_sink.h"

#include "utils.h"

static scribe_stdout_sink_t stdout_sink = SCRIBE_STDOUT_SINK_INITIALIZER();
#define STDOUT_SINK_INDEX ((size_t)0)

static scribe_coap_sink_t coap_sink = SCRIBE_COAP_SINK_DEFAULT_INITIALIZER();
#define COAP_SINK_INDEX ((size_t)1)

#define SINKS_COUNT ((size_t)2)

static mutex_t scribe_coap_sink_mutex = MUTEX_INIT;
static ssize_t scribe_coap_header_bytesize = 0;

static scribe_sink_t *sinks[SINKS_COUNT] = {
    [STDOUT_SINK_INDEX] = (scribe_sink_t *)&stdout_sink,
    [  COAP_SINK_INDEX] = (scribe_sink_t *)&coap_sink,
};

#define SCRIBE_OUTPUT_RETURN_VALUE_MAX_LABEL  XIPFS_STR(INT_MIN)

#define SCRIBE_OUTPUT_RETURN_VALUE_BUFFER_SIZE \
(sizeof(SCRIBE_OUTPUT_RETURN_VALUE_MAX_LABEL))

static char scribe_output_return_value_buffer[SCRIBE_OUTPUT_RETURN_VALUE_BUFFER_SIZE] = {
    "\0"
};

static const char   scribe_output_empty_default_text[]   = "\"\"";
static const size_t scribe_output_empty_default_text_len =
    sizeof(scribe_output_empty_default_text) - 1;

#define SCRIBE_OUTPUT_EXE_PAYLOAD_FIELD "\"out\""
static const char   scribe_output_header[]   = "{\"exe\":{" SCRIBE_OUTPUT_EXE_PAYLOAD_FIELD ":";
static const size_t scribe_output_header_len = sizeof(scribe_output_header) - 1;


#define SCRIBE_OUTPUT_EXE_RETURN_VALUE_FIELD "\"res\""

static const char   scribe_output_footer_0[]   = "," SCRIBE_OUTPUT_EXE_RETURN_VALUE_FIELD ":";
static const size_t scribe_output_footer_0_len = sizeof(scribe_output_footer_0) - 1;

static const char   scribe_output_footer_1[]   = "}}";
static const size_t scribe_output_footer_1_len = sizeof(scribe_output_footer_1) - 1;

static const char  *scribe_output_footer[2] = {
    scribe_output_footer_0,
    scribe_output_footer_1,
};
static const size_t scribe_output_footer_len[2] = {
    scribe_output_footer_0_len,
    scribe_output_footer_1_len,
};

int scribe_output_initialize(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    scribe_code_t scribe_res = scribe_initialize(sinks, SINKS_COUNT);
    if (scribe_res != SCRIBE_CODE_OK) {
        printf("scribe_output_initialize: failure : %s(%d)\n",
               scribe_code_get_label(scribe_res), scribe_res);
        return -1;
    }

    return 0;
}

ssize_t scribe_output_prepare(coap_pkt_t *pdu, uint8_t *buf, size_t bytesize)
{
    ssize_t res = 0;

    scribe_coap_header_bytesize = 0;

    coap_block_slicer_t *slicer = &(coap_sink.members.slicer);

    mutex_lock(&scribe_coap_sink_mutex);

    coap_block2_init(pdu, slicer);

    res = gcoap_resp_init(pdu, buf, bytesize, COAP_CODE_CONTENT);
    if (res < 0) {
        printf("scribe_output_prepare: failed to initialize coap response : %zd\n", res);
        goto exit;
    }

    res = coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    if (res < 0) {
        printf("scribe_output_prepare: failed to add coap format option : %zd\n", res);
        goto exit;
    }

    res = coap_opt_add_block2(pdu, slicer, true);
    if (res < 0) {
        printf("scribe_output_prepare: failed to add block2 option : %zd\n", res);
        goto exit;
    }

    res = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);
    if (res < 0) {
        printf("scribe_output_prepare: failed to finish options : %zd\n", res);
        goto exit;
    }
    scribe_coap_header_bytesize = res;

    void *sinks_prepare_data[SINKS_COUNT] = {
        [STDOUT_SINK_INDEX] = NULL,
        [  COAP_SINK_INDEX] = pdu,
    };
    scribe_code_t scribe_res = scribe_prepare(sinks_prepare_data);
    if (scribe_res != SCRIBE_CODE_OK) {
        printf("scribe_output_prepare: scribe failure : %s (%d)\n",
               scribe_code_get_label(scribe_res), scribe_res);
        res = -scribe_res;
        goto exit;
    }

    scribe_res = scribe_write(scribe_output_header, scribe_output_header_len);
    if (scribe_res != SCRIBE_CODE_OK) {
        printf("scribe_output_prepare: failed to write "
               SCRIBE_OUTPUT_EXE_PAYLOAD_FIELD ": %s (%d)\n",
               scribe_code_get_label(scribe_res), scribe_res);
        res = -1;
        goto exit;
    }
    res = scribe_output_header_len + scribe_coap_header_bytesize;
exit :
    /* printf("scribe_output_prepare: exit : %zd\n", res); */
    if (res < 0) {
        printf("scribe_output_prepare: exit on error : %zd\n", res);
        scribe_coap_header_bytesize = 0;
        mutex_unlock(&scribe_coap_sink_mutex);
    }
    return res;
}

ssize_t scribe_output_commit(int exe_return_value)
{
    ssize_t res = 0;
    scribe_code_t scribe_res;

    if (scribe_get_written_bytes_count() == scribe_output_header_len) {
        /* Executable wrote nothing to sinks as payload, let's push an empty default text then */
        scribe_res = scribe_write(scribe_output_empty_default_text,
                                  scribe_output_empty_default_text_len);
        if (scribe_res != SCRIBE_CODE_OK) {
            printf("scribe_output_commit: failed to write default empty text '%s' : %s (%d)\n",
                   scribe_output_empty_default_text,
                   scribe_code_get_label(scribe_res), scribe_res);
            res = -1;
            goto commit_and_exit;
        }
    }

    scribe_res = scribe_write(scribe_output_footer[0], scribe_output_footer_len[0]);
    if (scribe_res != SCRIBE_CODE_OK) {
        printf("scribe_output_commit: failed to write "
               SCRIBE_OUTPUT_EXE_RETURN_VALUE_FIELD " (1/3) : %s (%d)\n",
               scribe_code_get_label(scribe_res), scribe_res);
        res = -1;
        goto commit_and_exit;
    }

    res = snprintf(scribe_output_return_value_buffer,
                   sizeof(scribe_output_return_value_buffer),
                   "%d", exe_return_value);
    if ((res < 0) || ((res >= (ssize_t)sizeof(scribe_output_return_value_buffer)))) {
        printf("scribe_output_commit: failed to snprintf "
               SCRIBE_OUTPUT_EXE_RETURN_VALUE_FIELD " (2/3) : %zd\n", res);
        if (res >= (ssize_t)sizeof(scribe_output_return_value_buffer)) {
            res = -1;
        }
        goto commit_and_exit;
    }

    scribe_res = scribe_write(scribe_output_return_value_buffer, res);
    if (scribe_res != SCRIBE_CODE_OK) {
        printf("scribe_output_commit: failed to write "
               SCRIBE_OUTPUT_EXE_RETURN_VALUE_FIELD " (2/3) : %s (%d)\n",
               scribe_code_get_label(scribe_res), scribe_res);
        res = -1;
        goto commit_and_exit;
    }

    scribe_res = scribe_write(scribe_output_footer[1], scribe_output_footer_len[1]);
    if (scribe_res != SCRIBE_CODE_OK) {
        printf("scribe_output_commit: failed to write "
               SCRIBE_OUTPUT_EXE_RETURN_VALUE_FIELD " (3/3) : %s (%d)\n",
               scribe_code_get_label(scribe_res), scribe_res);
        res = -1;
        goto commit_and_exit;
    }

commit_and_exit:
    size_t written_bytes_count = 0;
    scribe_res = scribe_commit(&written_bytes_count);
    if (scribe_res != SCRIBE_CODE_OK) {
        printf("scribe_output_commit: scribe failure : %s (%d)\n",
               scribe_code_get_label(scribe_res), scribe_res);
        if (res >= 0) {
            res  = -1;
        }
        goto exit;
    }

    if (res >= 0) {
        res = scribe_coap_header_bytesize + written_bytes_count;
    }

exit :
    /* printf("scribe_output_commit: exit : %zd\n", res); */

    if (res < 0) {
        printf("scribe_output_commit: exit on error : %zd\n", res);
    }

    scribe_coap_header_bytesize = 0;
    mutex_unlock(&scribe_coap_sink_mutex);

    printf("\n");

    return res;
}

void scribe_output_release(void)
{
    scribe_release();
}
