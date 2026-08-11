/*
 * SPDX-FileCopyrightText: 2026 Université de Lille
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       An application demonstrating xipfs and coap usage.
 *
 * @author      Gregory Guche <gregory.guche@univ-lille.fr>
 *
 * @remarks Code and Makefile have been adpated from :
 *          - examples/networking/gnrc/border_router
 *          - examples/networking/coap/gcoap_fileserver
 * @}
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <inttypes.h>

#include "kernel_defines.h"
#include "net/gcoap.h"
#include "net/nanocoap/fileserver.h"
#include "shell.h"
#include "vfs_default.h"
#include "fs/xipfs_fs.h"

#include "create_exe.h"
#include "execute.h"
#include "scribe_output.h"

#define MAIN_QUEUE_SIZE (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#define NVME0P0_PAGE_NUM 10

XIPFS_NEW_PARTITION(nvm0, "/nvm0", NVME0P0_PAGE_NUM);

static
XIPFS_START_PARTITION_INCLUSION(nvm1)
#include "blob/nvm1.flash.h"
XIPFS_END_PARTITION_INCLUSION(nvm1, "/nvm1",nvm1_flash, nvm1_flash_len);

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
        "/nvm1",
        COAP_GET | COAP_MATCH_SUBTREE,
        nanocoap_fileserver_handler, "/nvm1"
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
            printf("gcoap fileserver: DEBUG: path: %s\n", ctx->path);
            if (path_len > 4) {
                if ( (ctx->path[path_len - 1] == 'e') &&
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

/**
 * @internal
 *
 * @brief Initialize a VFS XiPFS mountpoint.
 *
 * @param[in]   mp The mountpoint to initialize.
 *
 * @retval <0 on errors
 * @retval >=0 otherwise
 */
static int init_mount_point(vfs_xipfs_mount_t *mp)
{
    if (vfs_mount(&mp->vfs_mp) < 0) {
        printf("Error: vfs_mount: \"%s\": file system has not been "
            "initialized or is corrupted\n", mp->vfs_mp.mount_point);
        return -1;
    }

    printf("vfs_mount: \"%s\": OK\n", mp->vfs_mp.mount_point);
    return 0;
}

/**
 * @internal
 *
 * @brief Initialize all example mountpoints.
 */
static void init_mount_points(void)
{
    vfs_xipfs_mount_t *mps[2] = {
        [0] = &nvm0,
        [1] = &nvm1,
    };
    const size_t mps_count = ARRAY_SIZE(mps);
    for (size_t i = 0; i < mps_count; i++) {
        if (init_mount_point(mps[i]) < 0) {
            /* PANIC */
            for(;;) {}
        }
    }
}

static int initialize(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    init_mount_points();

    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
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

static void shutdown(int argc, const char *argv[]) {
    (void)argc;
    (void)argv;

    scribe_output_release();
}

int main(int argc, const char *argv[])
{
    int res = initialize(argc, argv);
    if (res != 0) {
        return res;
    }
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    shutdown(argc, argv);
    return 0;
}
