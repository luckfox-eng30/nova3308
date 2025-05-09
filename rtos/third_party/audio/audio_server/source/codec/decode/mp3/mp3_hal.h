/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#ifndef _AUDIO_MP3_HAL_H_
#define _AUDIO_MP3_HAL_H_

#include "AudioConfig.h"

#include "mp3dec.h"
#include "mp3common.h"

#define MP3_DECODE_FRAME_COUNT 2
#define MP3_ID3V2_HEADER_LENGHT (10)
#define MP3_AB_CORE_SHARE_ADDR_INVALID (0xdeadfcfc)

typedef struct _MP3PLAYERINFO
{
    HMP3Decoder mpi_mp3dec;
    MP3FrameInfo mpi_frameinfo;
} MP3PLAYERINFO;

int32_t AudioDecMP3Close(void);
int32_t AudioDecMP3Open(uint32_t A2B_ShareDatAddr);
int32_t AudioDecMP3Process(uint32_t decode_dat);

#endif        // _AUDIO_MP3_HAL_H_
