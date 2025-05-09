/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#ifndef __EQ_H__
#define __EQ_H__

rk_err_t rk_eq_init();
rk_err_t rk_eq_deinit();
void RockEQAdjust(long SmpRate, short *g, short db, RKEffect *userEQ); // db ΪԤ��˥����db ��, 0: ˥��0dB; 1:˥��6dB; 2:˥��12dB;
void RockEQProcess(short *pData, long PcmLen, RKEffect *userEQ);
#endif
