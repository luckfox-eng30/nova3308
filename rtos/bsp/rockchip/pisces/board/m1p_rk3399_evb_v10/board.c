/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-10     Cliff      first implementation
 *
 */

#include <rthw.h>
#include <rtthread.h>

#include "board.h"
#include "drv_cache.h"
#include "hal_base.h"
#include "hal_bsp.h"

#ifdef RT_USING_CRU
#include "drv_clock.h"
#endif
#ifdef RT_USING_I2C
#include "drv_i2c.h"
#endif
#ifdef RT_USING_PIN
#include "iomux.h"
#endif
#ifdef RT_USING_UART
#include "drv_uart.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
#include "drv_regulator.h"
#endif
#ifdef RT_USING_AUDIO
#include "rk_audio.h"
#endif
#ifdef RT_USING_PM
#include "drv_pm.h"
#endif

#ifdef RT_USING_CRU
static const struct clk_init clk_inits[] =
{
    INIT_CLK("SCLK_SHRM", SCLK_SHRM, 10 * MHZ),
    INIT_CLK("PCLK_SHRM", PCLK_SHRM, 10 * MHZ),
    INIT_CLK("PCLK_ALIVE", PCLK_ALIVE, 10 * MHZ),
    INIT_CLK("HCLK_ALIVE", HCLK_ALIVE, 10 * MHZ),
    INIT_CLK("HCLK_M4", HCLK_M4, 10 * MHZ),
    INIT_CLK("ACLK_LOGIC", ACLK_LOGIC, 10 * MHZ),
    INIT_CLK("HCLK_LOGIC", HCLK_LOGIC, 10 * MHZ),
    INIT_CLK("PCLK_LOGIC", PCLK_LOGIC, 10 * MHZ),
    INIT_CLK("CLK_SPI1", CLK_SPI1, 5 * MHZ),
    INIT_CLK("PLL_GPLL", PLL_GPLL, 1188 * MHZ),
    INIT_CLK("PLL_CPLL", PLL_CPLL, 1000 * MHZ),
    INIT_CLK("SCLK_SFC1_SRC", SCLK_SFC1_SRC, 132 * MHZ),
    INIT_CLK("HCLK_M4", HCLK_M4, 300 * MHZ),
    INIT_CLK("ACLK_DSP", ACLK_DSP, 400 * MHZ),
    INIT_CLK("ACLK_LOGIC", ACLK_LOGIC, 300 * MHZ),
    INIT_CLK("HCLK_LOGIC", HCLK_LOGIC, 150 * MHZ),
    INIT_CLK("PCLK_LOGIC", PCLK_LOGIC, 150 * MHZ),
    INIT_CLK("SCLK_SHRM", SCLK_SHRM, 300 * MHZ),
    INIT_CLK("PCLK_SHRM", PCLK_SHRM, 100 * MHZ),
    INIT_CLK("PCLK_ALIVE", PCLK_ALIVE, 100 * MHZ),
    INIT_CLK("HCLK_ALIVE", HCLK_ALIVE, 100 * MHZ),
    INIT_CLK("CLK_SPI1", CLK_SPI1, 50 * MHZ),
    { /* sentinel */ },
};

static const struct clk_unused clks_unused[] =
{
    {0, 0, 0x00030003},
    {0, 2, 0x58045804},
    {0, 5, 0x00ee00ee},
    {0, 6, 0x048d048d},
    {0, 7, 0x00110011},
    {0, 9, 0x40004000},
    {0, 11, 0x40e040e0},
    {0, 12, 0x90769076},
    {0, 13, 0xffffffff},
    {0, 14, 0xfefffeff},
    { /* sentinel */ },
};
#endif

#ifdef RT_USING_AUDIO
const struct audio_card_desc rk_board_audio_cards[] =
{
#ifdef RT_USING_AUDIO_CARD_ANALOG_MIC
    {
        .name = "sound0",
        .dai = I2STDM0,
        .vad = VAD,
        .codec = ACDCDIG,
        .capture = true,
        .mclkfs = 2048,
        .format = AUDIO_FMT_I2S,
    },
#endif
#ifdef RT_USING_AUDIO_CARD_I2S_MIC
    {
        .name = "sound0",
        .dai = I2STDM0,
        .vad = VAD,
        .codec = NULL,
        .capture = true,
        .mclkfs = 256,
        .format = AUDIO_FMT_I2S,
    },
#endif
#ifdef RT_USING_AUDIO_CARD_PDM_MIC
    {
        .name = "sound0",
        .dai = PDM0,
        .vad = VAD,
        .codec = NULL,
        .capture = true,
        .format = AUDIO_FMT_PDM,
    },
#endif
    { /* sentinel */ }
};
#endif

#ifdef PRINT_CLK_SUMMARY_INFO
/**
 *
 */
void print_clk_summary_info(void)
{
}
#endif

static void tick_isr(int vector, void *param)
{
    /* enter interrupt */
    rt_interrupt_enter();

    HAL_IncTick();
    rt_tick_increase();
#ifdef TICK_TIMER
    HAL_TIMER_ClrInt(TICK_TIMER);
#endif

    /* leave interrupt */
    rt_interrupt_leave();
}

static void mpu_init(void)
{
    static const ARM_MPU_Region_t table[] =
    {
        {
            .RBAR = ARM_MPU_RBAR(0U, 0x04000000U),
            .RASR = ARM_MPU_RASR(0U, ARM_MPU_AP_FULL, 0U, 0U, 1U, 0U, 0U, ARM_MPU_REGION_SIZE_1MB)
        },
        {
            .RBAR = ARM_MPU_RBAR(1U, 0x20000000U),
            .RASR = ARM_MPU_RASR(0U, ARM_MPU_AP_FULL, 0U, 0U, 1U, 0U, 0U, ARM_MPU_REGION_SIZE_1MB)
        },
        {
            .RBAR = ARM_MPU_RBAR(2U, 0x40000000U),
            .RASR = ARM_MPU_RASR(1U, ARM_MPU_AP_FULL, 0U, 0U, 0U, 0U, 0U, ARM_MPU_REGION_SIZE_256MB)
        },
        {
            .RBAR = ARM_MPU_RBAR(3U, 0x60000000U),
            .RASR = ARM_MPU_RASR(1U, ARM_MPU_AP_FULL, 0U, 0U, 0U, 0U, 0U, ARM_MPU_REGION_SIZE_256MB)
        },
    };

    ARM_MPU_Load(&(table[0]), 4U);

#ifdef RT_USING_UNCACHE_HEAP
    ARM_MPU_Region_t uncache_region;

    uncache_region.RBAR = ARM_MPU_RBAR(4U, RK_UNCACHE_HEAP_START);
    uncache_region.RASR = ARM_MPU_RASR(1U, ARM_MPU_AP_FULL, 0U, 0U, 0U, 0U, 0U, RT_UNCACHE_HEAP_ORDER);
    ARM_MPU_SetRegionEx(4, uncache_region.RBAR, uncache_region.RASR);
#endif

    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
}

#ifdef RT_USING_I2C
const struct rockchip_i2c_config rockchip_i2c_config_table[] =
{
    {
        .id = I2C0,
        .speed = I2C_100K,
    },
    { /* sentinel */ }
};
#endif

#if defined(RT_USING_UART0)
const struct uart_board g_uart0_board =
{
    .baud_rate = ROCKCHIP_UART_BAUD_RATE_DEFAULT,
    .dev_flag = ROCKCHIP_UART_SUPPORT_FLAG_DEFAULT,
    .bufer_size = RT_SERIAL_RB_BUFSZ,
    .name = "uart0",
};
#endif /* RT_USING_UART0 */

#if defined(RT_USING_UART1)
const struct uart_board g_uart1_board =
{
    .baud_rate = ROCKCHIP_UART_BAUD_RATE_DEFAULT,
    .dev_flag = ROCKCHIP_UART_SUPPORT_FLAG_DEFAULT,
    .bufer_size = RT_SERIAL_RB_BUFSZ,
    .name = "uart1",
};
#endif /* RT_USING_UART1 */

#ifdef HAL_PWR_MODULE_ENABLED
static struct regulator_desc regulators[] =
{
    {
        .flag = REGULATOR_FLG_INTREG,
        .desc.intreg_desc = {
            .flag = DESC_FLAG_LINEAR(PWR_FLG_VOLT_SSPD | PWR_FLG_VOLT_ST),
            .info = {
                .pwrId = PWR_ID_CORE,
            },
            PWR_INTREG_SHIFT_RUN(&PMU->LDO_CON[1], PMU_LDO_CON1_MCU_LDOCORE_SFT_SHIFT),
            PWR_INTREG_SHIFT_SSPD(&PMU->LDO_CON[1], PMU_LDO_CON1_PWRMODE_LDOCORE_ADJ_SHIFT),
            PWR_INTREG_SHIFT_ST(&PMU->LDO_STAT, PMU_LDO_STAT_LDO_CORE_ADJ_SHIFT),
            .voltMask = PMU_LDO_CON1_MCU_LDOCORE_SFT_MASK >> PMU_LDO_CON1_MCU_LDOCORE_SFT_SHIFT,
                    PWR_DESC_LINEAR_VOLT(750000, 1100000, 50000),
        },
    },
    {
        .flag = REGULATOR_FLG_INTREG,
        .desc.intreg_desc = {
            .flag = DESC_FLAG_LINEAR(PWR_FLG_VOLT_SSPD | PWR_FLG_VOLT_ST | PWR_FLG_PWR_EN),
            .info = {
                .pwrId = PWR_ID_VCC_MIPI,
            },
            PWR_INTREG_SHIFT_RUN(&PMU->LDO_CON[1], PMU_LDO_CON1_MCU_LDOMIPI_SFT_SHIFT),
            PWR_INTREG_SHIFT_SSPD(&PMU->LDO_CON[1], PMU_LDO_CON1_PWRMODE_LDOMIPI_ADJ_SHIFT),
            PWR_INTREG_SHIFT_EN(&PMU->LDO_CON[0], PMU_LDO_CON0_LDO_AUDIO_EN_SHIFT),
            PWR_INTREG_SHIFT_ST(&PMU->LDO_STAT, PMU_LDO_STAT_LDO_MIPI_ADJ_SHIFT),
            .voltMask = PMU_LDO_CON1_MCU_LDOMIPI_SFT_MASK >> PMU_LDO_CON1_MCU_LDOMIPI_SFT_SHIFT,
                    PWR_DESC_LINEAR_VOLT(750000, 1100000, 50000),
        },
    },
    {
        .flag = REGULATOR_FLG_INTREG,
        .desc.intreg_desc = {
            .flag = DESC_FLAG_LINEAR(PWR_FLG_PWR_EN),
            .info = {
                .pwrId = PWR_ID_VCC_AUDIO,
            },
            PWR_INTREG_SHIFT_RUN(&PMU->LDO_CON[0], PMU_LDO_CON0_LDO_AUDIO_SFT_SHIFT),
            PWR_INTREG_SHIFT_EN(&PMU->LDO_CON[0], PMU_LDO_CON0_LDO_AUDIO_EN_SHIFT),
            .voltMask = PMU_LDO_CON0_LDO_AUDIO_SFT_MASK >> PMU_LDO_CON0_LDO_AUDIO_SFT_SHIFT,
                    PWR_DESC_LINEAR_VOLT(1500000, 1650000, 50000),
        },
    },
    {
        .flag = REGULATOR_FLG_INTREG,
        .desc.intreg_desc = {
            .flag = DESC_FLAG_LINEAR(PWR_FLG_VOLT_SSPD),
            .info = {
                .pwrId = PWR_ID_DSP_CORE,
            },
            PWR_INTREG_SHIFT_RUN(&PMU->LDO_CON[2], PMU_LDO_CON2_DSP_LDOCORE_SFT_SHIFT),
            PWR_INTREG_SHIFT_SSPD(&PMU->LDO_CON[2], PMU_LDO_CON2_DSPAPM_LDOCORE_ADJ_SHIFT),
            .voltMask = PMU_LDO_CON2_DSP_LDOCORE_SFT_MASK >> PMU_LDO_CON2_DSP_LDOCORE_SFT_SHIFT,
                    PWR_DESC_LINEAR_VOLT(750000, 1100000, 50000),
        },
    },
    {
        .flag = REGULATOR_FLG_INTREG,
        .desc.intreg_desc = {
            .flag = DESC_FLAG_LINEAR(PWR_FLG_VOLT_SSPD),
            .info = {
                .pwrId = PWR_ID_DSP_VCC_MIPI,
            },
            PWR_INTREG_SHIFT_RUN(&PMU->LDO_CON[2], PMU_LDO_CON2_DSP_LDOMIPI_SFT_SHIFT),
            PWR_INTREG_SHIFT_SSPD(&PMU->LDO_CON[2], PMU_LDO_CON2_DSPAPM_LDOMIPI_ADJ_SHIFT),
            .voltMask = PMU_LDO_CON2_DSP_LDOMIPI_SFT_MASK >> PMU_LDO_CON2_DSP_LDOMIPI_SFT_SHIFT,
                    PWR_DESC_LINEAR_VOLT(750000, 1100000, 50000),
        },
    },
    { /* sentinel */ },
};

const struct regulator_init regulator_inits[] =
{
    REGULATOR_INIT("vdd_dsp_core", PWR_ID_DSP_CORE, 800000, 1, 800000, 1),
    REGULATOR_INIT("vcc_audio", PWR_ID_VCC_AUDIO, 0, 0, 0, 1),
    { /* sentinel */ },
};
#endif

#ifdef RT_USING_PM_REQ_PWR
static uint32_t core_pwr_req[3];
#define CORE_PWR_REQ_CNT 3
static struct req_pwr_desc req_pwr_array[] =
{
    {
        .pwr_id = PWR_ID_CORE,
        .req_ctrl = {
            .info.ttl_req = CORE_PWR_REQ_CNT, /* for core & shrm */
            .req_vals = &core_pwr_req[0],
        }
    },
    { /* sentinel */ },
};
#endif

#ifdef RT_USING_PM_DVFS
const static struct dvfs_table dvfs_core_table[] =
{
    {
        .freq = 99000000,
        .volt = 900000,
    },
    {
        .freq = 198000000,
        .volt = 900000,
    },
    { /* sentinel */ },
};

const static struct dvfs_table dvfs_shrm_table[] =
{
    {
        .freq = 99000000,
        .volt = 900000,
    },
    {
        .freq = 198000000,
        .volt = 900000,
    },
    { /* sentinel */ },
};

const static struct dvfs_table dvfs_dsp_table[] =
{
    {
        .freq = 49500000,
        .volt = 900000,
    },
    {
        .freq = 99000000,
        .volt = 900000,
    },
    {
        .freq = 198000000,
        .volt = 900000,
    },
    { /* sentinel */ },
};

struct rk_dvfs_desc dvfs_data[] =
{
    {
        .clk_id = SCLK_SHRM,
        .pwr_id = PWR_ID_CORE,
        .tbl_idx = 1,
        .table = &dvfs_shrm_table[0],
    },
    {
        .clk_id = HCLK_M4,
        .pwr_id = PWR_ID_CORE,
        .tbl_idx = 1,
        .table = &dvfs_core_table[0],
    },
    {
        .clk_id = ACLK_DSP,
        .pwr_id = PWR_ID_CORE,
        .tbl_idx = 2,
        .table = &dvfs_dsp_table[0],
    },
    { /* sentinel */ },
};

static struct pm_mode_dvfs pm_mode_data[] =
{
    {
        .clk_id = HCLK_M4,
        .run_tbl_idx = { 1, 1, 1 },
        .sleep_tbl_idx = 1,
    },
    {
        .clk_id = SCLK_SHRM,
        .run_tbl_idx = { 1, 1, 1 },
        .sleep_tbl_idx = 1,
    },
    { /* sentinel */ },
};
#endif

/**
 * This function will initial Pisces board.
 */
void rt_hw_board_init()
{
    mpu_init();

    /* HAL_Init */
    HAL_Init();

    /* tick init */
    HAL_SetTickFreq(1000 / RT_TICK_PER_SECOND);
    rt_hw_interrupt_install(TICK_IRQn, tick_isr, RT_NULL, "tick");
    rt_hw_interrupt_umask(TICK_IRQn);
#ifdef RT_USING_SYSTICK
    HAL_SYSTICK_CLKSourceConfig(HAL_TICK_CLKSRC_EXT);
    HAL_SYSTICK_Config((PLL_INPUT_OSC_RATE / RT_TICK_PER_SECOND) - 1);
    HAL_SYSTICK_Enable();
#else
    HAL_TIMER_Init(TICK_TIMER, TIMER_FREE_RUNNING);
    HAL_TIMER_SetCount(TICK_TIMER, (PLL_INPUT_OSC_RATE / RT_TICK_PER_SECOND) - 1);
    HAL_TIMER_Start_IT(TICK_TIMER);
#endif

    rt_hw_cpu_cache_init();

#ifdef RT_USING_PIN
    rt_hw_iomux_config();
#endif

#ifdef RT_USING_CRU
    clk_init(clk_inits, true);
    /* disable some clks when init, and enabled by device when needed */
    clk_disable_unused(clks_unused);
    if (RT_CONSOLE_DEVICE_UART(0))
        CRU->CRU_CLKGATE_CON[2] = 0x08860886;
    else if (RT_CONSOLE_DEVICE_UART(1))
        CRU->CRU_CLKGATE_CON[2] = 0x080d080d;
    else
        CRU->CRU_CLKGATE_CON[2] = 0x088f088f;
#endif

#ifdef RT_USING_PMU
    HAL_PD_Off(PD_AUDIO);
#endif

    rk_rt_pm_init();

    /* Initial usart deriver, and set console device */
#ifdef RT_USING_UART
    rt_hw_usart_init();
#endif

#ifdef RT_USING_CONSOLE
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

    /* Print clk summary info */
#ifdef PRINT_CLK_SUMMARY_INFO
    print_clk_summary_info();
#endif

#ifdef HAL_PWR_MODULE_ENABLED
    regulator_desc_init(regulators);
#endif

#ifdef RT_USING_PM_REQ_PWR
    regulator_req_desc_init(req_pwr_array);
#endif

#ifdef RT_USING_PM_DVFS
    dvfs_desc_init(dvfs_data);
    rkpm_register_dvfs_info(pm_mode_data, 0);
#endif

    /* hal bsp init */
    BSP_Init();

    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
