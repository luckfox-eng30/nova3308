/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronic Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-03-12     Jair Wu      First version
 *
 */

//#include "rkos.h"
#include <stdio.h>
#include <stdint.h>

typedef struct ByteIOContext
{
    uint8_t *buffer;
    int bye_index;
    int bit_left;
    int size_in_bits;
    void *userdata;
} ByteIOContext;

typedef struct ByteCache
{
    u32 cachedata;
    u8 cacheindex;
    void *userdata;
} ByteCache;

uint32_t get_le32(ByteIOContext *pb);
uint32_t get_le16(ByteIOContext *pb);
void url_fskip(ByteIOContext *pb, u16 len);
void url_fseek(ByteIOContext *pb, u16 len, u8 type);
void get_buffer(ByteIOContext *pb, u8 *buf, u8 len);
void freebuf(ByteIOContext *pb);
u8 get_bitbye(ByteIOContext *pb, u8 len);

void av_free();
void *av_malloc(int n);
u32 fread32(ByteCache *datas);
u16 fread16(ByteCache *datas);
u8 fread8(ByteCache *datas);
void fileout(u8 *outbuf, u16 len);
u32 Blockout(int32_t *decoded0, int32_t *decoded1, u8 *outbuf, u16 len, int ch, u32 bps);
#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
