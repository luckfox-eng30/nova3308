/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#include <stdio.h>
#include <math.h>
#include <dfs_posix.h>

#include "drv_heap.h"
#include "drv_display.h"
#include "hal_base.h"

#include "app_main.h"

static struct image_st target;

static img_load_info_t img_qrcode_bkg = { DISP_XRES, DISP_YRES, USERDATA_PATH"page_qrcode.dta"};

rt_err_t app_qrcode_init(void *param)
{
    struct image_st *par;
    rt_err_t ret = RT_EOK;

    if (param)
    {
        par = (struct image_st *)param;
        target = *par;
    }

    return ret;
}

rt_err_t app_qrcode_design(void *param)
{
    struct image_st *par = (struct image_st *)&target;
    rt_uint32_t vir_w = par->stride / format2depth[par->format];

    return app_load_img(&img_qrcode_bkg, (rt_uint8_t *)par->pdata,
                        vir_w, par->height, 0, format2depth[par->format]);
}
