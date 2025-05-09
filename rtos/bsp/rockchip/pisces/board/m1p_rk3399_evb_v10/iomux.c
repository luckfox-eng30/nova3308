/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  ******************************************************************************
  * @file    iomux.c
  * @version V0.1
  * @brief   iomux for M1
  *
  * Change Logs:
  * Date           Author          Notes
  * 2019-05-25     Cliff.Chen      first implementation
  *
  ******************************************************************************
  */

/** @addtogroup RKBSP_Board_Driver
 *  @{
 */

/** @addtogroup IOMUX
 *  @{
 */

/** @defgroup How_To_Use How To Use
 *  @{
 @verbatim

 ==============================================================================
                    #### How to use ####
 ==============================================================================
 This file provide IOMUX for board, it will be invoked when board initialization.

 @endverbatim
 @} */
#include "rtdef.h"
#include "iomux.h"
#include "hal_base.h"

/********************* Private MACRO Definition ******************************/
/** @defgroup IOMUX_Private_Macro Private Macro
 *  @{
 */

/** @} */  // IOMUX_Private_Macro

/********************* Private Structure Definition **************************/
/** @defgroup IOMUX_Private_Structure Private Structure
 *  @{
 */

/** @} */  // IOMUX_Private_Structure

/********************* Private Variable Definition ***************************/
/** @defgroup IOMUX_Private_Variable Private Variable
 *  @{
 */

/** @} */  // IOMUX_Private_Variable

/********************* Private Function Definition ***************************/
/** @defgroup IOMUX_Private_Function Private Function
 *  @{
 */

/** @} */  // IOMUX_Private_Function

/********************* Public Function Definition ****************************/

/** @defgroup IOMUX_Public_Functions Public Functions
 *  @{
 */

/**
 * @brief  Config iomux for PDM
 */
static void pdm_iomux_config(void)
{
    // for pdm input
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_A0 |  // PDM_IN_CLK0
                         GPIO_PIN_A1 |  // PDM_IN_CLK1
                         GPIO_PIN_A2 |  // PDM_IN_SDI0
                         GPIO_PIN_A3,   // PDM_IN_SDI1
                         PIN_CONFIG_MUX_FUNC1);
    // for pdm output
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_A6 |  // PDM_OUT_CLK
                         GPIO_PIN_A7 |  // PDM_OUT_CLK1
                         GPIO_PIN_B0 |  // PDM_OUT_SDI0
                         GPIO_PIN_B1,   // PDM_OUT_SDI1
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for I2S
 */
RT_UNUSED static void i2s_iomux_config(void)
{
    // for i2s input
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_A0 |  // I2S_IN_SCLK
                         GPIO_PIN_A1 |  // I2S_IN_LRCK
                         GPIO_PIN_A2 |  // I2S_IN_SDI0
                         GPIO_PIN_A3,   // I2S_IN_SDI1
                         PIN_CONFIG_MUX_FUNC2);
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_A4,  //I2S_IN_MCLK
                         PIN_CONFIG_MUX_FUNC1);
    // for i2s output
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_A6 |  // I2S_OUT_SCLK
                         GPIO_PIN_A7 |  // I2S_OUT_LRCK
                         GPIO_PIN_B0 |  // I2S_OUT_SDO0
                         GPIO_PIN_B1,   // I2S_OUT_SDO1
                         PIN_CONFIG_MUX_FUNC2);
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_A5,  //I2S_OUT_MCLK
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for UART0
 */
RT_UNUSED static void uart0_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_C7 |  // UART0_RX
                         GPIO_PIN_D0 |  // UART0_TX
                         GPIO_PIN_D1 |  // UART0_CTS
                         GPIO_PIN_D2,   // UART0_RTS
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for UART1
 */
RT_UNUSED static void uart1_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_D1 |  // UART1_RX_MUX0
                         GPIO_PIN_D2,   // UART1_TX_MUX0
                         PIN_CONFIG_MUX_FUNC2);
}

/**
 * @brief  Config iomux for CPU JTAG
 */
RT_UNUSED static void m4_jtag_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_C7 |  // M4_JTAG1_TCK
                         GPIO_PIN_D0,   // M4_JTAG1_TMS
                         PIN_CONFIG_MUX_FUNC2);
}

/**
 * @brief  Config iomux for SPI master
 */
RT_UNUSED static void spi_master_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_C2 |  // SPIMST1_CS
                         GPIO_PIN_C3 |  // SPIMST1_CLK
                         GPIO_PIN_C4 |  // SPIMST1_MISO
                         GPIO_PIN_C5,   // SPIMST1_MOSI
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for i2c0
 */
static void i2c0_master_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_C0 |  // I2CMST0_SCL
                         GPIO_PIN_C1,   // I2CMST0_SDA
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for DSP JTAG
 */
RT_UNUSED static void dsp_jtag_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_C0 |  // DSP_JTAG0_TCK
                         GPIO_PIN_C1 |  // DSP_JTAG0_TMS
                         GPIO_PIN_C2 |  // DSP_JTAG0_TRSTN
                         GPIO_PIN_C3 |  // DSP_JTAG0_TDI
                         GPIO_PIN_C4,   // DSP_JTAG0_TDO
                         PIN_CONFIG_MUX_FUNC3);
}

/**
 * @brief  Config iomux for touch panel
 */
static void tp_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_D3 |  // TP_RESETN (Touch Panel Reset)
                         GPIO_PIN_D4,   // TP_INTN (Touch Panel Interrupt)
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for SPI slave
 */
static void spi_slave_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_B2 |  // SPISLV0_CS
                         GPIO_PIN_B3 |  // SPISLV0_CLK
                         GPIO_PIN_B4 |  // SPISLV0_MOSI
                         GPIO_PIN_B5,   // SPISLV0_MISO
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for I2C slave
 */
static void i2c_slave_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_B6 |  // I2CSLV_SCL
                         GPIO_PIN_B7,   // I2CSLV_SCL
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for LCD
 */
static void lcd_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK1,
                         GPIO_PIN_A0 |  // LCD_IN_RESETN (LCD Reset from AP)
                         GPIO_PIN_A1 |  // LCD_IN_TE (LCD TE from AP)
                         GPIO_PIN_A2 |  // LCD_OUT_RESETN (Reset to LCD)
                         GPIO_PIN_A3 |  // LCD_OUT_TE (TE to LCD)
                         GPIO_PIN_A4,   // LDO_OUT_PWR_EN (Power Enable to External LDO)
                         PIN_CONFIG_MUX_FUNC1);
}

/**
 * @brief  Config iomux for wakeup
 */
static void wakeup_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK0,
                         GPIO_PIN_D7,   // OLPC_AP_INT (Interrupt to AP)
                         PIN_CONFIG_MUX_FUNC1);

    HAL_PINCTRL_SetIOMUX(GPIO_BANK1,
                         GPIO_PIN_B1,
                         PIN_CONFIG_MUX_FUNC0); //AP WAKEUP M1 by gpio interrupt
}

/**
 * @brief  Config iomux for SFC1
 */
RT_WEAK RT_UNUSED void sfc1_iomux_config(void)
{
    HAL_PINCTRL_SetIOMUX(GPIO_BANK1,
                         GPIO_PIN_C4 |  // SFC1_CS
                         GPIO_PIN_C5 |  // SFC1_CLK
                         GPIO_PIN_C6 |  // SFC1_D0
                         GPIO_PIN_C7,   // SFC1_D1
                         PIN_CONFIG_MUX_FUNC2);

    HAL_PINCTRL_SetIOMUX(GPIO_BANK1,
                         GPIO_PIN_C2 |  // SFC1_D3
                         GPIO_PIN_C3,   // SFC1_D2
                         PIN_CONFIG_MUX_FUNC3);
}
/**
 * @brief  Config iomux for board of M1
 */
void rt_hw_iomux_config(void)
{
    pdm_iomux_config();
#ifdef M4_JTAG_ENABLE
    m4_jtag_iomux_config();
#else
    uart0_iomux_config();
#endif

#ifdef RT_USING_UART1
    uart1_iomux_config();
#endif

#ifdef DSP_JTAG_ENABLE
    dsp_jtag_iomux_config();
#else
    spi_master_iomux_config();
    i2c0_master_iomux_config();
#endif
    tp_iomux_config();
    spi_slave_iomux_config();
    i2c_slave_iomux_config();
    lcd_iomux_config();
    wakeup_iomux_config();
#ifdef RT_USING_QPIPSRAM_FSPI_HOST
    sfc1_iomux_config();
#endif
}

/** @} */  // IOMUX_Public_Functions

/** @} */  // IOMUX

/** @} */  // RKBSP_Board_Driver
