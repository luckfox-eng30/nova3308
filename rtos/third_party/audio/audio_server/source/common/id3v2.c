/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronic Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-03-16     Jair Wu      First version
 *
 */

#include "AudioConfig.h"

struct id3v2_header
{
    char id3[3];
    char mainVer;
    char subVer;
    char flag;
    char size[4];
};

static int get_ID3V2_len(char buf[4])
{
    int Len = ((buf[0] & 0x7f) << 21) | ((buf[1] & 0x7f) << 14) |
              ((buf[2] & 0x7f) << 7 ) | ((buf[3] & 0x7f));

    return Len;
}

int check_ID3V2_tag(char *buf)
{
    struct id3v2_header *id3v2 = (struct id3v2_header *)buf;

    if (strncmp(id3v2->id3, "ID3", 3))
        return 0;

    if (id3v2->subVer > 3)
        return 0;

    return get_ID3V2_len(id3v2->size);
}
