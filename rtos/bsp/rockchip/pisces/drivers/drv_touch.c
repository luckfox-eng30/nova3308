/**************************************************************
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd
 * VENDOR_EDIT
 * File             : drv_touch.c
 * Description      : Source file for olpc touch driver
 * Version          : 1.0
 * Date             : 2019-05-13
 * Author           : zhoufeng@swdp
 * ---------------- Revision History: --------------------------
 *   <version>        <date>                  < author >                                                        <desc>
 * Revision 1.1, 2019-05-13, zhoufeng@swdp, initial olpc touch driver
 ****************************************************************/
#include <rtthread.h>
#include <soc.h>
#include <rtdevice.h>
#include <hal_base.h>

#ifdef RT_USING_PISCES_TOUCH

#include "drv_touch.h"
#include "drv_touch_s3706.h"

rt_err_t rt_touch_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct rt_touch_device *touch;

    RT_ASSERT(dev != RT_NULL);

    touch = (struct rt_touch_device *)dev;

    if (touch->ops->open(touch, oflag) != RT_EOK)
    {
        return RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t rt_touch_close(rt_device_t dev)
{
    struct rt_touch_device *touch;

    RT_ASSERT(dev != RT_NULL);

    touch = (struct rt_touch_device *)dev;

    if (touch->ops->close(touch) != RT_EOK)
    {
        return RT_ERROR;
    }

    return RT_EOK;
}

rt_size_t rt_touch_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    struct rt_touch_device *touch;
    rt_size_t ret = 0;

    RT_ASSERT(dev != RT_NULL);

    touch = (struct rt_touch_device *)dev;

    ret = touch->ops->read(touch, pos, buffer, size);

    return ret;
}

rt_err_t rt_touch_control(rt_device_t dev, int cmd, void *args)
{
    struct rt_touch_device *touch;

    RT_ASSERT(dev != RT_NULL);

    touch = (struct rt_touch_device *)dev;

    if (touch->ops->control(touch, cmd, args) != RT_EOK)
    {
        return RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t rt_hw_touch_register(struct rt_touch_device *touch,
                              const char *name,
                              rt_uint32_t flag,
                              void *data)
{
    rt_uint32_t ret;
    struct rt_device *device;
    RT_ASSERT(touch != RT_NULL);

    device = &(touch->parent);

    device->type        = RT_Device_Class_Miscellaneous;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;

    device->init        = RT_NULL;
    device->open        = rt_touch_open;
    device->close       = rt_touch_close;
    device->read        = rt_touch_read;
    device->write       = RT_NULL;
    device->control     = rt_touch_control;
    device->user_data   = data;

    ret = rt_device_register(device, name, flag);

    return ret;
}

#endif /* RT_USING_PISCES_TOUCH */

