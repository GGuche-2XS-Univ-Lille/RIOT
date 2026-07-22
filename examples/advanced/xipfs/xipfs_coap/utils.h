/*
 * SPDX-FileCopyrightText: 2026 Université de Lille
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef XIPFS_COAP_UTILS_H
#define XIPFS_COAP_UTILS_H

#include <stdint.h>

#ifndef XIPFS_COAP_DEFAULT_SEPARATOR
#define XIPFS_COAP_DEFAULT_SEPARATOR ' '
#endif

static inline uint16_t find_next_param_separator_ex(const char *from,
                                                    uint16_t begin, uint16_t end,
                                                    char separator) {
    for (uint16_t i = begin; i < end; i++) {
        if (from[i] == separator)
            return i;
    }

    return end;
}

static inline uint16_t find_next_param_separator(const char *from,
                                                 uint16_t begin, uint16_t end) {
    return find_next_param_separator_ex(from, begin, end,
                                        XIPFS_COAP_DEFAULT_SEPARATOR);
}

#endif /* XIPFS_COAP_UTILS_H */
