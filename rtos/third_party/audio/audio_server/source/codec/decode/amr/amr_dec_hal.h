/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#ifndef _AUDIO_AMR_DEC_HAL_H_
#define _AUDIO_AMR_DEC_HAL_H_
#include "AudioConfig.h"

#include <stddef.h>
#include <stdio.h>

enum amr_type
{
    AMR_NB = 0,
    AMR_WB
};

#define SAMPLE_PER_FRAME     160
#define SAMPLE_PER_FRAME_WB  320

/**
 * According to RFC-4867 Document
 * https://tools/ietf.org/html/rfc4867
 */
#define AMR_HEADER_AMRNB_SC "#!AMR\n"
#define AMR_HEADER_AMRWB_SC "#!AMR-WB\n"
#define AMR_HEADER_AMRNB_MC "#!AMR_MC1.0\n"
#define AMR_HEADER_AMRWB_MC "#!AMR-WB_MC1.0\n"

#define AMR_HEADER_AMRNB_SC_LENGTH 6
#define AMR_HEADER_AMRWB_SC_LENGTH 9
#define AMR_HEADER_AMRNB_MC_LENGTH 11
#define AMR_HEADER_AMRWB_MC_LENGTH 14

struct amr_dec
{
    void               *_decoder;
    unsigned char       _buffer_in[256];
    short               _buffer_out[SAMPLE_PER_FRAME_WB * 2];
    int                 is_wb;
    int                 is_mono;
    struct audio_server_data_share *ab_share_dat;
};

int32_t AudioDecAmrClose(void);
int32_t AudioDecAmrOpen(uint32_t A2B_ShareDatAddr);
int32_t AudioDecAmrProcess(uint32_t decode_dat);

#endif        // _AUDIO_AMR_DEC_HAL_H_
