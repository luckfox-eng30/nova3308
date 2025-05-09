/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  ******************************************************************************
  * @file    i2c_test.c
  * @author  David Wu
  * @version V0.1
  * @date    20-Mar-2019
  * @brief   i2c test for pisces
  *
  ******************************************************************************
  */

#include <rtdevice.h>
#include <rtthread.h>

#ifdef RT_USING_COMMON_TEST_TOUCH

#include <stdbool.h>
#include "hal_base.h"

#include <drv_touch.h>

static rt_device_t g_touch_dev = RT_NULL;

void touch_show_usage()
{
    /* touch test */
    rt_kprintf("touch test.\n");
}

void touch_test(int argc, char **argv)
{
    if (argc > 1)
        goto out;

    /* register touch device for lvgl */
    g_touch_dev = rt_device_find("s3706");
    RT_ASSERT(g_touch_dev != RT_NULL);

    rt_device_control(g_touch_dev, TOUCH_CMD_EXSEM, RT_NULL);

    return;
out:
    touch_show_usage();
    return;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT(touch_test, touch test cmd);
#endif

#endif
