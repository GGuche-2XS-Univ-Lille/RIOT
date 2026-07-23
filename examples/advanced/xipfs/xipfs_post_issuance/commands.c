#include "commands.h"

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include "fs/xipfs_fs.h"
#include "periph/flashpage.h"
#include "shell.h"
#include "vfs.h"

static int convert(const char *str, uint32_t *val)
{
    char *endptr;
    long l;

    errno = 0;

    l = strtol(str, &endptr, 10);

    if (l == LONG_MIN && errno != 0)
        return -1;

    if (l == LONG_MAX && errno != 0)
        return -1;

    if (endptr == str)
        return -1;

    if ((long unsigned int)l > UINT32_MAX)
        return -1;

    if (l < 0)
        return -1;

    if (*endptr != '\0')
        return -1;

    *val = (uint32_t)l;

    return 0;
}

static char full_path_buffer[XIPFS_PATH_MAX];

static const char *make_full_path(const char *filename, char buffer[XIPFS_PATH_MAX])
{
    int res = snprintf(buffer, XIPFS_PATH_MAX, "/nvme0p1/%s", filename);
    if ((res < 0) || (res >= XIPFS_PATH_MAX))
    {
        return NULL;
    }

    return buffer;
}

static uint32_t bytes_to_load_count = 0;
static uint32_t offset = 0;

int mkbin_callback(int argc, char **argv)
{
    uint32_t size, exec;

    if (argc < 4) {
        printf("%s: name size exec\n", argv[0]);
        return 1;
    }

    const char *full_path = make_full_path(argv[1], full_path_buffer);
    if (full_path == NULL) {
        fprintf(stderr, "%s: %s: failed to make full path\n", argv[0], argv[1]);
        return 1;
    }

    int fd = vfs_open(full_path, 0, 0);
    if (fd >= 0) {
        fprintf(stderr, "%s: %s: file name already used\n", argv[0],
            full_path);
        vfs_close(fd);
        return 1;
    }

    if (convert(argv[2], &size) != 0) {
        fprintf(stderr, "%s: %s: invalid size\n", argv[0], full_path);
        return 1;
    }

    if (convert(argv[3], &exec) != 0) {
        fprintf(stderr, "%s: %s: invalid rights\n", argv[0], full_path);
        return 1;
    }

    if (exec != 0 && exec != 1) {
        fprintf(stderr, "%s: %s: invalid rights\n", argv[0], full_path);
        return 1;
    }

    int res = xipfs_extended_driver_new_file(full_path, size, exec);
    if (res < 0) {
        fprintf(stderr, "%s: %s: unable to create file\n", argv[0], full_path);
        return 1;
    }

    bytes_to_load_count = size;
    offset = 0;

    return 0;
}

static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


static int
valid(char c)
{
    if (c >= 'A' && c <= 'Z')
        return 0;
    if (c >= 'a' && c <= 'z')
        return 0;
    if (c >= '0' && c <= '9')
        return 0;
    if (c == '+' || c == '/' || c == '=')
        return 0;
    return -1;
}

static int
check_chunk(const char *chunk)
{
    size_t i, len;

    len = strlen(chunk);

    if (len == 0)
        return -1;

    if ((len & 3) != 0)
        return -1;

    for (i = 0; i < len; i++)
        if (valid(chunk[i]) != 0)
            return -1;

    return 0;
}

static int b64decode(int fd, const char *chunk, size_t len)
{
    uint32_t i = 0, bytes = 0, r = 3;
    const char *ptr;
    char buf[3];
    int res;

    while (i < len && bytes_to_load_count > 0) {
        ptr = strchr(b64, chunk[i]);
        bytes |= (ptr - b64) << 18;

        ptr = strchr(b64, chunk[i+1]);
        bytes |= (ptr - b64) << 12;

        ptr = strchr(b64, chunk[i+2]);
        if (ptr != NULL)
            bytes |= (ptr - b64) << 6;
        else
            r--;

        ptr = strchr(b64, chunk[i + 3]);
        if (ptr != NULL)
            bytes |= ptr - b64;
        else
            r--;

        buf[0] = (bytes >> 16) & 0xff;
        buf[1] = (bytes >>  8) & 0xff;
        buf[2] = (bytes      ) & 0xff;

        res = vfs_write(fd, buf, r);
        if (res < 0) {
            return -1;
        }

        offset += r;
        bytes_to_load_count -= r;
        bytes = 0;
        i += 4;
        r = 3;
    }

    return 0;
}

int ldbin_callback(int argc, char **argv)
{
    if (argc < 3) {
        printf("%s: name chunk\n", argv[0]);
        return 1;
    }

    const char *full_path = make_full_path(argv[1], full_path_buffer);
    if (full_path == NULL) {
        fprintf(stderr, "%s: %s: failed to make full path\n", argv[0], argv[1]);
        return 1;
    }

    if (bytes_to_load_count == 0) {
        printf("%s: no chunk expected for '%s'\n", argv[0], full_path);
        return 1;
    }

    int fd = vfs_open(full_path, O_WRONLY | O_APPEND, 0);
    if (fd < 0) {
        fprintf(stderr, "%s: %s: no such file\n", argv[0], full_path);
        return 1;
    }

    if (check_chunk(argv[2]) != 0) {
        fprintf(stderr, "%s: %s: invalid chunk\n", argv[0], full_path);
        return 1;
    }

    int res = b64decode(fd, argv[2], strlen(argv[2]));

    vfs_close(fd);

    if (res < 0) {
        fprintf(stderr, "%s: %s: failed to write chunk\n", argv[0], full_path);
        bytes_to_load_count = 0;
        offset = 0;
        return 1;
    }

    if (bytes_to_load_count == 0) {
        offset = 0;
    }

    return 0;
}
