/**************************************************************
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd
 * VENDOR_EDIT
 * File             : drv_touch.h
 * Description      : Head file for olpc touch driver
 * Version          : 1.0
 * Date             : 2019-05-13
 * Author           : zhoufeng@swdp
 * ---------------- Revision History: --------------------------
 *   <version>        <date>                  < author >                                                        <desc>
 * Revision 1.1, 2019-05-13, zhoufeng@swdp, initial olpc touch driver
 ****************************************************************/
#ifndef __DRV_TOUCH_H__
#define __DRV_TOUCH_H__

#include <stdint.h>
#include <rtdevice.h>

//#define TP_DEBUG    1

#ifdef TP_DEBUG
#define tp_dbg(...) \
    do { \
        rt_kprintf("%s: ", __func__);    \
        rt_kprintf(__VA_ARGS__);         \
    } while(0)
#else
#define tp_dbg(...)
#endif

#define TP_RST_CTL_REG      0x40050824
#define TP_RST_VAL_LOW      0X02000200
#define TP_RST_VAL_HIGH     0X01000100

/* Register addresses */
#define STMPE811_CHIP_ID        0x00
#define STMPE811_ID_VER         0x02
#define STMPE811_SYS_CTRL1      0x03
#define STMPE811_SYS_CTRL2      0x04
#define STMPE811_SPI_CFG        0x08
#define STMPE811_INT_CTRL       0x09
#define STMPE811_INT_EN         0x0A
#define STMPE811_INT_STA        0x0B
#define STMPE811_GPIO_EN        0x0C
#define STMPE811_GPIO_INT_STA   0x0D
#define STMPE811_ADC_INT_EN     0x0E
#define STMPE811_ADC_INT_STA    0x0F
#define STMPE811_GPIO_SET_PIN   0x10
#define STMPE811_GPIO_CLR_PIN   0x11
#define STMPE811_GPIO_MP_STA    0x12
#define STMPE811_GPIO_DIR       0x13
#define STMPE811_GPIO_ED        0x14
#define STMPE811_GPIO_RE        0x15
#define STMPE811_GPIO_FE        0x16
#define STMPE811_GPIO_AF        0x17
#define STMPE811_ADC_CTRL1      0x20
#define STMPE811_ADC_CTRL2      0x21
#define STMPE811_ADC_CAPT       0x22
#define STMPE811_ADC_DATA_CH0   0x30
#define STMPE811_ADC_DATA_CH1   0x32
#define STMPE811_ADC_DATA_CH2   0x34
#define STMPE811_ADC_DATA_CH3   0x36
#define STMPE811_ADC_DATA_CH4   0x38
#define STMPE811_ADC_DATA_CH5   0x3A
#define STMPE811_ADC_DATA_CH6   0x3C
#define STMPE811_ADC_DATA_CH7   0x3E
#define STMPE811_TSC_CTRL       0x40
#define STMPE811_TSC_CFG        0x41
#define STMPE811_WDW_TR_X       0x42
#define STMPE811_WDW_TR_Y       0x44
#define STMPE811_WDW_BL_X       0x46
#define STMPE811_WDW_BL_Y       0x48
#define STMPE811_FIFO_TH        0x4A
#define STMPE811_FIFO_STA       0x4B
#define STMPE811_FIFO_SIZE      0x4C
#define STMPE811_TSC_DATA_X     0x4D
#define STMPE811_TSC_DATA_Y     0x4F
#define STMPE811_TSC_DATA_Z     0x51
#define STMPE811_TSC_FRACTION_Z 0x56
#define STMPE811_TSC_DATA_XYZ   0x57
#define STMPE811_TSC_DATA       0xD7
#define STMPE811_TSC_I_DRIVE    0x58
#define STMPE811_TSC_SHIELD     0x59
#define STMPE811_TEMP_CTRL      0x60
#define STMPE811_TEMP_DATA      0x61
#define STMPE811_TEMP_TH        0x62

typedef enum TOUCH_CTL_CMD
{
    TOUCH_CMD_EXSEM      = 0x00,
} touch_ctl_cmd;


/* Touch state */
struct touch_state
{
    int16_t x;                            ///< Position X
    int16_t y;                            ///< Position Y
    uint8_t pressed;                      ///< Pressed flag
    uint8_t padding;
};

struct point_info
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint8_t  width_major;
    uint8_t  touch_major;
    uint8_t  status;
};

struct rt_touch_device;

/**
 * touch operations
 */
struct rt_touch_ops
{
    rt_err_t (*open)(struct rt_touch_device *touch_device, rt_uint16_t oflag);
    rt_err_t (*close)(struct rt_touch_device *touch_device);
    rt_size_t (*read)(struct rt_touch_device *touch_device, rt_off_t pos, void *buffer, rt_size_t size);
    rt_err_t (*control)(struct rt_touch_device *touch_device, int cmd, void *arg);
};

/**
 * touch device
 */
struct rt_touch_device
{
    struct rt_device parent;
    const struct rt_touch_ops *ops;
};

/* Exported functions ------------------------------------------------------- */

rt_err_t rt_hw_touch_register(struct rt_touch_device *touch,
                              const char *name,
                              rt_uint32_t flag,
                              void *data);

#endif /* __DRV_TOUCH_H__ */

