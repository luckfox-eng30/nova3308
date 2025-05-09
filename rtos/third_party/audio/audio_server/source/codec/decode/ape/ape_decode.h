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

#ifndef _APE_DECODE_H_
#define _APE_DECODE_H_

#include "AudioConfig.h"
#include "ape.h"

#ifdef AUDIO_DECODER_APE

#define APE_BUFFER_IN               (4096 * 2)

int32_t ape_dec_init(APEDec **dec);
int32_t ape_dec_process(APEDec *dec);
int32_t ape_dec_deinit(APEDec *dec);
#endif  // AUDIO_DECODER_APE

#endif  // _APE_DECODE_H_
