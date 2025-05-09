/**************************************************************
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd
 * VENDOR_EDIT
 * File             : drv_touch_s3706.c
 * Description      : Source file for olpc touch driver
 * Version          : 1.0
 * Date             : 2019-05-14
 * Author           : zhoufeng@swdp
 * ---------------- Revision History: --------------------------
 *   <version>        <date>                  < author >                                                        <desc>
 * Revision 1.1, 2019-05-14, zhoufeng@swdp, initial olpc touch driver for s3706
 ****************************************************************/
#include <rtthread.h>
#include <soc.h>
#include <hal_base.h>
#include <rthw.h>

#ifdef RT_USING_PISCES_TOUCH

#include <drv_touch.h>
#include <drv_touch_s3706.h>

static touch_device_t g_touch_dev;

static void print_s3706_reg(touch_device_t *dev)
{
    tp_dbg("s3706_reg.F12_2D_QUERY_BASE = 0x%x.\n", dev->reg.F12_2D_QUERY_BASE);
    tp_dbg("s3706_reg.F12_2D_CMD_BASE = 0x%x.\n", dev->reg.F12_2D_CMD_BASE);
    tp_dbg("s3706_reg.F12_2D_CTRL_BASE = 0x%x.\n", dev->reg.F12_2D_CTRL_BASE);
    tp_dbg("s3706_reg.F12_2D_DATA_BASE = 0x%x.\n", dev->reg.F12_2D_DATA_BASE);

    tp_dbg("s3706_reg.F01_RMI_QUERY_BASE = 0x%x.\n", dev->reg.F01_RMI_QUERY_BASE);
    tp_dbg("s3706_reg.F01_RMI_CMD_BASE = 0x%x.\n", dev->reg.F01_RMI_CMD_BASE);
    tp_dbg("s3706_reg.F01_RMI_CTRL_BASE = 0x%x.\n", dev->reg.F01_RMI_CTRL_BASE);
    tp_dbg("s3706_reg.F01_RMI_DATA_BASE = 0x%x.\n", dev->reg.F01_RMI_DATA_BASE);

    tp_dbg("s3706_reg.F34_FLASH_QUERY_BASE = 0x%x.\n", dev->reg.F34_FLASH_QUERY_BASE);
    tp_dbg("s3706_reg.F34_FLASH_CMD_BASE = 0x%x.\n", dev->reg.F34_FLASH_CMD_BASE);
    tp_dbg("s3706_reg.F34_FLASH_CTRL_BASE = 0x%x.\n", dev->reg.F34_FLASH_CTRL_BASE);
    tp_dbg("s3706_reg.F34_FLASH_DATA_BASE = 0x%x.\n", dev->reg.F34_FLASH_DATA_BASE);
}

static void s3706_int_handler(int vector, void *param)
{
    //HAL_NVIC_DisableIRQ(TP_IRQn);
    rt_interrupt_enter();

    rt_hw_interrupt_mask(TP_IRQn);

    rt_sem_release(g_touch_dev.irq_sem);

    rt_interrupt_leave();
    //HAL_NVIC_EnableIRQ(TP_IRQn);
}

static rt_err_t s3706_touch_read(touch_device_t *dev, rt_uint16_t slaveaddr, void *cmd_buf, size_t cmd_len, void *data_buf, size_t data_len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr  = slaveaddr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = cmd_buf;
    msgs[0].len   = cmd_len;

    msgs[1].addr  = slaveaddr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = data_buf;
    msgs[1].len   = data_len;

    if (rt_i2c_transfer(dev->i2c_bus, msgs, 2) == 2)
        return RT_EOK;
    else
        return RT_ERROR;
}

static rt_err_t s3706_touch_read_word(touch_device_t *dev, uint16_t addr, int32_t *value)
{
    rt_err_t retval = 0;
    uint16_t regaddr = addr;
    uint8_t buf[2] = {0};

    retval = s3706_touch_read(dev, dev->addr, &regaddr, 1, buf, 2);
    if (retval != RT_ERROR)
    {
        *value = buf[1] << 8 | buf[0];
        return RT_EOK;
    }
    else
    {
        *value = 0;
        return RT_ERROR;
    }
}

static rt_err_t s3706_touch_write(touch_device_t *dev, uint16_t slaveaddr, uint16_t regaddr, size_t cmd_len, uint32_t data_buf_arg, size_t data_len)
{
    char *data_buf = NULL;
    uint32_t data_buf_tmp;
    struct rt_i2c_msg msgs[1];
    int i, ret = 0;

    data_buf = (char *)rt_calloc(1, data_len + cmd_len);
    if (!data_buf)
    {
        tp_dbg("spi write alloc buf size %d fail\n", data_len);
        return RT_ERROR;
    }

    for (i = 0; i < cmd_len; i++)
    {
        data_buf[i] = (regaddr >> (8 * i)) & 0xff;
        tp_dbg("send[%x]: 0x%x\n", i, data_buf[i]);
    }

    for (i = cmd_len; i < data_len + cmd_len; i++)
    {
        data_buf_tmp = (uint8_t)(data_buf_arg & 0x000000ff);
        data_buf[i] = data_buf_tmp;
        data_buf_arg = data_buf_arg >> 8;
    }

    msgs[0].addr  = slaveaddr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = (rt_uint8_t *)data_buf;
    msgs[0].len   = data_len;

    ret = rt_i2c_transfer(dev->i2c_bus, msgs, 1);

    rt_free(data_buf);

    if (ret == 1)
        return RT_EOK;
    else
        return RT_ERROR;
}

static uint32_t s3706_trigger_reason(touch_device_t *dev, int32_t gesture_enable, int32_t is_suspended)
{
    rt_err_t ret_code = 0;
    int32_t ret_value = 0;
    uint8_t device_status = 0, interrupt_status = 0;
    uint32_t result_event = 0;

    ret_code = s3706_touch_write(dev, dev->addr, 0xff, 1, 0, 1);  /* page 0*/
    if (ret_code != RT_EOK)
    {
        tp_dbg("s3706_touch_write error, ret_code = %d\n", ret_code);
        return IRQ_EXCEPTION;
    }

    ret_code = s3706_touch_read_word(dev, dev->reg.F01_RMI_DATA_BASE, &ret_value);
    if (ret_code != RT_EOK)
    {
        tp_dbg("s3706_touch_write error, ret_code = %d\n", ret_code);
        return IRQ_EXCEPTION;
    }
    device_status = ret_value & 0xff;
    interrupt_status = (ret_value & 0x7f00) >> 8;
    tp_dbg("interrupt_status = 0x%x, device_status = 0x%x\n", interrupt_status, device_status);

    if (device_status && device_status != 0x81)
    {
        tp_dbg("interrupt_status = 0x%x, device_status = 0x%x\n", interrupt_status, device_status);
        return IRQ_EXCEPTION;
    }

    if (interrupt_status & 0x04)
    {
        if (gesture_enable && is_suspended)
        {
            return IRQ_GESTURE;
        }
        else if (is_suspended)
        {
            return IRQ_IGNORE;
        }
        TP_SET_BIT(result_event, IRQ_TOUCH);
        TP_SET_BIT(result_event, IRQ_FINGERPRINT);
    }
    if (interrupt_status & 0x10)
    {
        TP_SET_BIT(result_event, IRQ_BTN_KEY);
    }
    if (interrupt_status & 0x20)
    {
        TP_SET_BIT(result_event, IRQ_FACE_STATE);
        TP_SET_BIT(result_event, IRQ_FW_HEALTH);
    }

    return result_event;
}

static int32_t s3706_get_touch_points(touch_device_t *dev, struct point_info *points, int32_t max_num)
{
    rt_err_t ret_code = 0;
    int32_t i, obj_attention;
    uint8_t fingers_to_process = max_num;
    char *buf;

    buf = dev->point_buf;
    memset(buf, 0, sizeof(dev->point_buf));

    ret_code = s3706_touch_read_word(dev, dev->reg.F12_2D_DATA15, &obj_attention);
    if (ret_code != RT_EOK)
    {
        rt_kprintf("s3706_touch_read_word error, ret_code = %d\n", ret_code);
        return -RT_ERROR;
    }
    tp_dbg("obj_attention = 0x%x\n", obj_attention);

    for (i = 9;  ; i--)
    {
        if ((obj_attention & 0x03FF) >> i  || i == 0)
            break;
        else
            fingers_to_process--;
    }
    tp_dbg("fingers to process = 0x%x\n", fingers_to_process);

    ret_code = s3706_touch_read(dev, dev->addr, &dev->reg.F12_2D_DATA_BASE, 1, buf, 8 * fingers_to_process);
    if (ret_code == RT_ERROR)
    {
        rt_kprintf("touch i2c read block failed\n");
        return -RT_ERROR;
    }
    tp_dbg("buf[0] = 0x%x, buf[1]= 0x%x, buf[2] = 0x%x.\n", buf[0], buf[1], buf[2]);

    rt_mutex_take(dev->read_mutex, RT_WAITING_FOREVER);
    for (i = 0; i < fingers_to_process; i++)
    {
        points[i].x = ((buf[i * 8 + 2] & 0x0f) << 8) | (buf[i * 8 + 1] & 0xff);
        points[i].y = ((buf[i * 8 + 4] & 0x0f) << 8) | (buf[i * 8 + 3] & 0xff);
        points[i].z = buf[i * 8 + 5];
        points[i].touch_major = max(buf[i * 8 + 6], buf[i * 8 + 7]);
        points[i].width_major = ((buf[i * 8 + 6] & 0x0f) + (buf[i * 8 + 7] & 0x0f)) / 2;
        points[i].status = buf[i * 8];
    }
    rt_mutex_release(dev->read_mutex);

    return obj_attention;
}

static rt_err_t s3706_touch_handle(touch_device_t *dev)
{
    rt_err_t ret = RT_EOK;
    int obj_attention = 0;

    obj_attention = s3706_get_touch_points(dev, dev->points, dev->max_finger);
    if (obj_attention == -EINVAL)
    {
        tp_dbg("Invalid points, ignore..\n");
        return RT_ERROR;
    }

    tp_dbg("points: x = 0x%x, y = 0x%x, z = 0x%x.\n", dev->points[0].x, dev->points[0].y, dev->points[0].z);

#ifdef RT_USING_PISCES_TOUCH_ASYNC
    ret = rt_mq_send(&dev->tp_mq, dev->points, POINT_INFO_LEN);
    RT_ASSERT(ret == RT_EOK);
#endif

    return ret;
}

static void touch_thread_entry(void *parameter)
{
    uint32_t cur_event = 0;

    touch_device_t *dev = (touch_device_t *)parameter;

    rt_thread_delay(100); // wait for synaptics touch device to startup!

    while (1)
    {
        if (rt_sem_take(dev->irq_sem, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }

        tp_dbg("touch thread take sem.\n");

        cur_event = s3706_trigger_reason(dev, 0, 0);

        if (TP_CHK_BIT(cur_event, IRQ_TOUCH) || TP_CHK_BIT(cur_event, IRQ_BTN_KEY) || TP_CHK_BIT(cur_event, IRQ_FW_HEALTH) || \
                TP_CHK_BIT(cur_event, IRQ_FACE_STATE) || TP_CHK_BIT(cur_event, IRQ_FINGERPRINT))
        {
            if (TP_CHK_BIT(cur_event, IRQ_BTN_KEY))
            {
                // TODO tp_btnkey_handle(ts);
            }
            if (TP_CHK_BIT(cur_event, IRQ_TOUCH))
            {
                s3706_touch_handle(dev);
            }
            if (TP_CHK_BIT(cur_event, IRQ_FW_HEALTH) /*&& (!ts->is_suspended)*/)
            {
                // TODO health_monitor_handle(ts);
            }
            if (TP_CHK_BIT(cur_event, IRQ_FACE_STATE) /*&& ts->fd_enable*/)
            {
                // TODO tp_face_detect_handle(ts);
            }
            if (TP_CHK_BIT(cur_event, IRQ_FINGERPRINT) /*&& ts->fp_enable*/)
            {
                // TODO tp_fingerprint_handle(ts);
            }
        }
        else if (TP_CHK_BIT(cur_event, IRQ_GESTURE))
        {
            // TODO tp_gesture_handle(ts);
        }
        else if (TP_CHK_BIT(cur_event, IRQ_EXCEPTION))
        {
            // TODO tp_exception_handle(ts);
        }
        else if (TP_CHK_BIT(cur_event, IRQ_FW_CONFIG))
        {
            // TODO tp_config_handle(ts);
        }
        else if (TP_CHK_BIT(cur_event, IRQ_FW_AUTO_RESET))
        {
            // TODO tp_fw_auto_reset_handle(ts);
        }
        else
        {
            tp_dbg("unknown irq trigger reason.\n");
        }

        rt_hw_interrupt_umask(TP_IRQn);
    }
}

/*
 * s3706 initialization
 */
rt_err_t s3706_touch_device_init(touch_device_t *dev)
{
    uint16_t regaddr = 0;
    char *data_buf = NULL;
    uint32_t data_len = 0;

    dev->i2c_bus = (struct rt_i2c_bus_device *)rt_i2c_bus_device_find("i2c0");
    if (dev->i2c_bus == RT_NULL)
    {
        tp_dbg("Did not find device: i2c bus ...\n");
        return RT_ERROR;
    }

    /* page 0 */
    s3706_touch_write(dev, dev->addr, 0xff, 1, 0, 1);

    /* read register info */
    data_len = 4;
    data_buf = (char *)rt_calloc(1, data_len);

    /* read register info */
    regaddr = 0xdd;
    s3706_touch_read(dev, dev->addr, &regaddr, 1, data_buf, data_len);
    dev->reg.F12_2D_QUERY_BASE = data_buf[0];
    dev->reg.F12_2D_CMD_BASE = data_buf[1];
    dev->reg.F12_2D_CTRL_BASE = data_buf[2];
    dev->reg.F12_2D_DATA_BASE = data_buf[3];

    regaddr = 0xe3;
    s3706_touch_read(dev, dev->addr, &regaddr, 1, data_buf, data_len);
    dev->reg.F01_RMI_QUERY_BASE = data_buf[0];
    dev->reg.F01_RMI_CMD_BASE = data_buf[1];
    dev->reg.F01_RMI_CTRL_BASE = data_buf[2];
    dev->reg.F01_RMI_DATA_BASE = data_buf[3];

    regaddr = 0xe9;
    s3706_touch_read(dev, dev->addr, &regaddr, 1, data_buf, data_len);
    dev->reg.F34_FLASH_QUERY_BASE = data_buf[0];
    dev->reg.F34_FLASH_CMD_BASE = data_buf[1];
    dev->reg.F34_FLASH_CTRL_BASE =  data_buf[2];
    dev->reg.F34_FLASH_DATA_BASE = data_buf[3];

    dev->reg.F01_RMI_QUERY11 = dev->reg.F01_RMI_QUERY_BASE + 0x0b;    // product id
    dev->reg.F01_RMI_CTRL00 = dev->reg.F01_RMI_CTRL_BASE;
    dev->reg.F01_RMI_CTRL01 = dev->reg.F01_RMI_CTRL_BASE + 1;
    dev->reg.F01_RMI_CTRL02 = dev->reg.F01_RMI_CTRL_BASE + 2;
    dev->reg.F01_RMI_CMD00  = dev->reg.F01_RMI_CMD_BASE;
    dev->reg.F01_RMI_DATA01 = dev->reg.F01_RMI_DATA_BASE + 1;

    dev->reg.F12_2D_CTRL08 = dev->reg.F12_2D_CTRL_BASE;               // max XY Coordinate
    dev->reg.F12_2D_CTRL23 = dev->reg.F12_2D_CTRL_BASE + 9;           //glove enable
    dev->reg.F12_2D_CTRL32 = dev->reg.F12_2D_CTRL_BASE + 0x0f;        //moisture enable
    dev->reg.F12_2D_DATA04 = dev->reg.F12_2D_DATA_BASE + 1;           //gesture type
    dev->reg.F12_2D_DATA15 = dev->reg.F12_2D_DATA_BASE + 3;           //object attention
    dev->reg.F12_2D_CMD00  = dev->reg.F12_2D_CMD_BASE;
    dev->reg.F12_2D_CTRL20 = dev->reg.F12_2D_CTRL_BASE + 0x07;        //motion suppression
    dev->reg.F12_2D_CTRL27 = dev->reg.F12_2D_CTRL_BASE + 0x0b;        // wakeup Gesture enable

    print_s3706_reg(dev);

    s3706_touch_write(dev, dev->addr, 0xff, 1, 0x4, 1); /* page 4*/

    /* read firmware info */
    regaddr = dev->reg.F34_FLASH_CTRL_BASE;
    tp_dbg("F34_FLASH_CTRL_BASE = 0x%x.\n", regaddr);
    s3706_touch_read(dev, dev->addr, &regaddr, 1, data_buf, 4);

#ifdef TP_DEBUG
    {
        uint32_t current_firmware = 0;
        current_firmware = (data_buf[0] << 24) | (data_buf[1] << 16) | (data_buf[2] << 8) | data_buf[3];
        tp_dbg("S3706 init CURRENT_FIRMWARE_ID = 0x%x\n", current_firmware);
    }
#endif

    rt_hw_interrupt_install(TP_IRQn,
                            (rt_isr_handler_t)s3706_int_handler,
                            RT_NULL,
                            RT_NULL);
    rt_hw_interrupt_umask(TP_IRQn);

    return RT_EOK;
}


rt_err_t s3706_touch_open(struct rt_touch_device *touch_device, rt_uint16_t oflag)
{
    return RT_EOK;
}


rt_err_t s3706_touch_close(struct rt_touch_device *touch_device)
{
    return RT_EOK;
}

rt_size_t s3706_touch_device_read(struct rt_touch_device *touch_device, rt_off_t pos, void *buffer, rt_size_t size)
{
    touch_device_t *touch_dev = (touch_device_t *)touch_device;

    rt_mutex_take(touch_dev->read_mutex, RT_WAITING_FOREVER);

    rt_memcpy(buffer, touch_dev->points, sizeof(struct point_info));

    rt_mutex_release(touch_dev->read_mutex);

    return sizeof(struct point_info);
}

rt_err_t s3706_touch_control(struct rt_touch_device *touch_device, int cmd, void *arg)
{
    return RT_EOK;
}

static const struct rt_touch_ops touch_ops =
{
    .open = s3706_touch_open,
    .close = s3706_touch_close,
    .read = s3706_touch_device_read,
    .control = s3706_touch_control,
};

static rt_err_t touch_register(touch_device_t *dev, char *touch_name)
{
    struct rt_touch_device *rt_touch;

    RT_ASSERT(dev);
    RT_ASSERT(touch_name);

    rt_touch = &dev->parent;
    rt_touch->ops = &touch_ops;

    /* create touch driver thread */

    rt_thread_t touch_thread;
    touch_thread = rt_thread_create("tpdrv",
                                    touch_thread_entry, dev,
                                    1024, 10, 20);
    if (touch_thread != RT_NULL)
        rt_thread_startup(touch_thread);


    return rt_hw_touch_register(rt_touch, touch_name, RT_DEVICE_FLAG_RDWR, dev);
}

int rt_hw_touch_init(void)
{
    rt_err_t result = 0;

    tp_dbg("enter rt_hw_touch_init\n");

    g_touch_dev.irq_sem = rt_sem_create("s3706_isr_sem", 0, RT_IPC_FLAG_FIFO);
    RT_ASSERT(g_touch_dev.irq_sem != RT_NULL);

    g_touch_dev.read_mutex = rt_mutex_create("s3706_read_mutex", RT_IPC_FLAG_FIFO);
    RT_ASSERT(g_touch_dev.read_mutex != RT_NULL);

#ifdef RT_USING_PISCES_TOUCH_ASYNC
    /* init message queue */

    result = rt_mq_init(&g_touch_dev.tp_mq,
                        "tp_mq",
                        &g_touch_dev.tp_msg_pool[0],        /* mem pool */
                        POINT_INFO_LEN,                     /* bytes per message */
                        sizeof(g_touch_dev.tp_msg_pool),    /* mem pool size */
                        RT_IPC_FLAG_FIFO);                  /* allocating message for waiting threads according to FIFO order */
    RT_ASSERT(result == RT_EOK);
#endif

    g_touch_dev.addr = S3706_ADDR;
    g_touch_dev.max_finger = MAX_FINGER_NUM;

    touch_register(&g_touch_dev, "s3706");

    s3706_touch_device_init(&g_touch_dev);

    return result;
}
INIT_ENV_EXPORT(rt_hw_touch_init);

#endif /* RT_USING_PISCES_TOUCH*/
