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
 * @remarks Code and Makefile have been adapted from :
 *          - examples/networking/gnrc/border_router
 *          - examples/networking/coap/gcoap_fileserver
 * @}
 */

#include <stdio.h>
#include <stdint.h>

#include "kernel_defines.h"
#include "shell.h"
#include "vfs_default.h"
#include "fs/xipfs_fs.h"

#include "coap_server.h"

#define MAIN_QUEUE_SIZE (4)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static
XIPFS_START_PARTITION_INCLUSION(nvm0)
#include "blob/nvm0.flash.h"
XIPFS_END_PARTITION_INCLUSION(nvm0, "/nvm0",nvm0_flash, nvm0_flash_len);

static int initialize(int argc, const char *argv[])
{
    vfs_xipfs_mount_t *mps[1] = {
        [0] = &nvm0,
    };

    const size_t mps_count = ARRAY_SIZE(mps);
    for (size_t i = 0; i < mps_count; i++) {
        if (vfs_mount(&(mps[i]->vfs_mp)) < 0) {
            printf("Error: vfs_mount: \"%s\": file system has not been "
                   "initialized or is corrupted\n", mps[i]->vfs_mp.mount_point);
            return -1;
        }

        printf("vfs_mount: \"%s\": OK\n", mps[i]->vfs_mp.mount_point);
    }

    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    return coap_server_initialize(argc, argv);
}

static void shutdown(int argc, const char *argv[]) {
    (void)argc;
    (void)argv;

    coap_server_shutdown();
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
    return res;
}
