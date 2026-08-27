#include "coap_server.h"

#include <stdint.h>

#include "net/gcoap.h"
#include "net/nanocoap/fileserver.h"

#include "fs/xipfs_fs.h"

#include "create_exe.h"
#include "execute.h"
#include "scribe_output.h"

/* CoAP resources. Must be sorted by path (ASCII order). */
static const coap_resource_t _resources[] = {
#ifdef XIPFS_COAP_CREATE_EXE_ENABLED
    {
        "/create_exe",
        COAP_PUT | COAP_MATCH_SUBTREE,
        create_exe_handler, NULL
    },
#endif /* XIPFS_COAP_CREATE_EXE_ENABLED */

    {
        "/execution",
        COAP_GET | COAP_MATCH_SUBTREE,
        execute_handler, NULL
    },

    {
        "/nvm0",
        COAP_GET |
#if IS_USED(MODULE_NANOCOAP_FILESERVER_PUT)
        COAP_PUT |
#endif
#if IS_USED(MODULE_NANOCOAP_FILESERVER_DELETE)
        COAP_DELETE |
#endif
        COAP_MATCH_SUBTREE,
        nanocoap_fileserver_handler, "/nvm0"
    },

    {
        "/safe_execution",
        COAP_GET | COAP_MATCH_SUBTREE,
        execute_protected_handler, NULL
    },
};

static gcoap_listener_t _listener = {
    .resources = _resources,
    .resources_len = ARRAY_SIZE(_resources),
};

static int _event_cb(nanocoap_fileserver_event_t event, nanocoap_fileserver_event_ctx_t *ctx)
{
    switch (event) {
        case NANOCOAP_FILESERVER_GET_FILE_START:
            printf("gcoap fileserver: Download started: %s\n", ctx->path);
            break;
        case NANOCOAP_FILESERVER_GET_FILE_END:
            printf("gcoap fileserver: Download finished: %s\n", ctx->path);
            break;
        case NANOCOAP_FILESERVER_PUT_FILE_START:
#ifndef XIPFS_COAP_CREATE_EXE_ENABLED
            if (xipfs_does_filename_belong_to_known_mountpoint(ctx->path)) {
                uint32_t exec_rights = 0;
                size_t path_len = strlen(ctx->path);
                /* printf("gcoap fileserver: DEBUG: path: %s\n", ctx->path); */
                if (path_len > 4) {
                    if ((ctx->path[path_len - 1] == 'e') &&
                        (ctx->path[path_len - 2] == 'a') &&
                        (ctx->path[path_len - 3] == 'f') &&
                        (ctx->path[path_len - 4] == '.') ) {
                            exec_rights = true;
                        }
                }
                int ret = xipfs_extended_driver_new_file( ctx->path,
                                                          ctx->total_bytesize,
                                                          exec_rights );
                if (ret < 0) {
                    printf("_event_cb: PUT: failed to create '%s' : error=%d\n",
                           ctx->path, ret);
                    return -1;
                }
            }
#endif
            printf("gcoap fileserver: Upload started: %s\n", ctx->path);
            break;
        case NANOCOAP_FILESERVER_PUT_FILE_END:
            printf("gcoap fileserver: Upload finished: %s\n", ctx->path);
            break;
        case NANOCOAP_FILESERVER_DELETE_FILE:
            printf("gcoap fileserver: Delete %s\n", ctx->path);
            break;
        default:
            printf("gcoap fileserver: unknown event: %d\n", event);
            return -1;
    }

    return 0;
}

int coap_server_initialize(int argc, const char *argv[])
{
    (void)argc;
    gcoap_register_listener(&_listener);

    if (IS_USED(MODULE_NANOCOAP_FILESERVER_CALLBACK)) {
        nanocoap_fileserver_set_event_cb(_event_cb, NULL);
    }

    int res = scribe_output_initialize(argc, argv);
    if (res < 0) {
        printf("%s:initialize: Failed to initialize scribe: %d\n", argv[0], res);
        return -res;
    }

    return 0;
}

void coap_server_shutdown(void)
{
    scribe_output_release();
}
