/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  ******************************************************************************
  * @file    color_palette.h
  * @version V0.1
  * @brief   create bpp format lookup table
  *
  * Change Logs:
  * Date           Author          Notes
  * 2019-02-20     Huang Jiachai   first implementation
  *
  ******************************************************************************
  */

#ifndef _COLOR_PALETTE_H_
#define _COLOR_PALETTE_H_

#include <rtthread.h>
#ifdef RT_USING_VOP

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define GAMMA_RED           1.0
#define GAMMA_GREEN         1.0
#define GAMMA_BLUE          1.0
#define FORMAT_RGB_332      0
#define FORMAT_BGR_233      1
#define FORMAT_INDEX8       2

#endif

#endif
