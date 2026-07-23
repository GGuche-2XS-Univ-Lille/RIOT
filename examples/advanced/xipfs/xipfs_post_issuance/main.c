/*
 * SPDX-FileCopyrightText: 2024-2026 Université de Lille
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       An application demonstrating xipfs usage from code.
 *
 * @author      Gregory Guche <gregory.guche@univ-lille.fr>
 *
 * @}
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>

#include "fs/xipfs_fs.h"
#include "periph/flashpage.h"
#include "shell.h"
#include "vfs.h"

#include "commands.h"

#define PANIC() for (;;) {} /**< This macro handles fatal errors */

/*
 * Include a mount point image that has been built on a PC workstation.
 *
 * Please mind the deliberate missing semicolon character after
 * XIPFS_START_PARTITION_INCLUSION(nvme0p0).
 */
static
XIPFS_START_PARTITION_INCLUSION(nvme0p0)
#include "blob/nvme0p0.flash.h"
XIPFS_END_PARTITION_INCLUSION(nvme0p0, "/nvme0p0", nvme0p0_flash, nvme0p0_flash_len);

#define NVME0P1_PAGE_NUM 10 /**< The number of flash page for the nvme0p1 file system. */

/* Allocate a new contiguous space for the nvme0p1 file system. */
XIPFS_NEW_PARTITION(nvme0p1, "/nvme0p1", NVME0P1_PAGE_NUM);

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
        [0] = &nvme0p0,
        [1] = &nvme0p1,
    };
    const size_t mps_count = ARRAY_SIZE(mps);
    for (size_t i = 0; i < mps_count; i++) {
        if (init_mount_point(mps[i]) < 0) {
            PANIC();
        }
    }
}

static shell_command_t shell_commands[] = {
    {"mkbin"    , "allocate the space needed to load a binary", mkbin_callback    },
    {"ldbin"    , "load a chunk of binary"                    , ldbin_callback    },
    {NULL, NULL, NULL}
};

int main(void)
{
    char line_buf[SHELL_DEFAULT_BUFSIZE];

    /* Mount both included and allocated filesystems. */
    init_mount_points();

    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
