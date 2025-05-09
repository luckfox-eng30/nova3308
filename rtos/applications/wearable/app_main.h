/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_MAIN__
#define __APP_MAIN__
#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dfs_posix.h>

#include "drivers/rtc.h"
#include "drv_heap.h"
#include "drv_display.h"
#include "image_info.h"
#include "display.h"

#if defined(RT_USING_TOUCH_DRIVERS)
#include "touch.h"
#include "touchpanel.h"
#endif

#include "image_info.h"
#include "audio_server.h"

#include "app_page.h"
#if defined(RT_USING_PANEL_AM018RT90211)
#include "ui_map_368x448.h"
#elif defined(RT_USING_PANEL_AM014RT90327V0)
#include "ui_map_452x454.h"
#else
#error "May not support this panel"
#endif

#include "unistd.h"

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */

#define APP_CLOCK_WIN_0         0
#define APP_CLOCK_WIN_1         1
#define APP_CLOCK_WIN_2         2

// mq cmd define
#define MQ_DESIGN_UPDATE            (0x201UL)
#define MQ_REFR_UPDATE              (0x202UL)
#define MQ_PAGE_REFR_UPDATE         (0x203UL)
#define MQ_BACKLIGHT_SWITCH         (0x204UL)

//event define
#define EVENT_REFR_DONE             (0x01UL << 0)

// touch define
#define TOUCH_START_THRESHOLD   25
#define TOUCH_MOVE_THRESHOLD    3
#define TOUCH_MOVE_STEP         30

#define TOUCH_DIR_MODE_NULL     0
#define TOUCH_DIR_MODE_LR       1   //move left & right
#define TOUCH_DIR_MODE_UPDN     2   //move up & down

#define USERDATA_PATH           "/sdcard/"
#define ANIM_SRCPATH            USERDATA_PATH"anim"
#define MUSIC_DIR_PATH          USERDATA_PATH"music"
#define MUSIC_ICONS_PATH        USERDATA_PATH"player"
#define ICONS_PATH              USERDATA_PATH"icons"
#define WATCH_PATH              USERDATA_PATH"watch"
#define WEATHER_PATH            USERDATA_PATH"weather"
#define NOTICE_PATH             USERDATA_PATH"notice"

#define APP_WDT_TIMEOUT             2       // s
#define APP_TIMING_LIGHTOFF_MIN     5       // s
#define APP_TIMING_LIGHTOFF         30      // s
#define APP_SUSPEND_RESUME_ENABLE   1
#define APP_SUSPEND_TIMEOUT         1000    // ms
#define APP_WDT_ENABLE              1

#ifdef RT_PSRAM_END_RESERVE
#define APP_PSRAM_END_RESERVE   RT_PSRAM_END_RESERVE
#else
#define APP_PSRAM_END_RESERVE   0
#endif

enum app_play_mode
{
    APP_PLAY_LIST = 1,
    APP_PLAY_LOOP,
    APP_PLAY_RANDOM,
};

enum app_pm_status
{
    APP_PM_NONE,
    APP_PM_SUSPEND,
    APP_PM_RESUME,
};

enum ver_page
{
    VER_PAGE_BOTTOM = -1,
    VER_PAGE_NULL   = 0,
    VER_PAGE_TOP    = 1,
};

/*
 **************************************************************************************************
 *
 * Struct & data define
 *
 **************************************************************************************************
 */
typedef struct
{
    rt_uint16_t w;
    rt_uint16_t h;
    const char *name;
} img_load_info_t;

typedef struct
{
    rt_uint16_t year;
    rt_uint8_t  month;
    rt_uint8_t  day;
    rt_uint8_t  weekdays;

    rt_uint8_t  hour;
    rt_uint8_t  minute;
    rt_uint8_t  second;
    rt_uint8_t  tick;
} clock_time_t;

#define CLOCK_APP_MQ_NUM    32
typedef struct
{
    rt_uint32_t cmd;
    void       *param;
} clock_app_mq_t;

typedef struct
{
    rt_slist_t list;
    rt_err_t (*cb)(void *param);
} design_cb_t;

typedef struct
{
    rt_err_t (*cb)(struct rt_display_config *wincfg, void *param);
    void *param;
} refresh_cb_t;

typedef struct
{
    rt_err_t (*cb)(void *param);
    void *param;
    int32_t timeout;
} app_clock_cb_t;

typedef struct
{
    rt_int32_t wait;
    rt_uint8_t wflag;
    rt_uint8_t id;
} refrsh_request_param_t;

typedef struct
{
    struct app_page_data_t *page;
    uint8_t page_num;
    uint8_t auto_resize;
    void (*cb)(int mov_fix);
} page_refrsh_request_param_t;

typedef struct
{
    rt_uint8_t win_id;
    rt_uint8_t win_layer;
} app_disp_refrsh_param_t;

typedef struct
{
    rt_int32_t offset;
    rt_int8_t dir;
} mov_design_param;

/* Do not over 32 bytes */
struct app_info
{
    rt_uint32_t magic;
    rt_uint32_t page_index;
    rt_uint32_t cold_boot;
};

struct app_touch_cb_t
{
    rt_err_t (*tp_touch_down)(void *param);

    rt_err_t (*tp_move_lr_start)(void *param);
    rt_err_t (*tp_move_updn_start)(void *param);

    rt_err_t (*tp_move_lr)(void *param);
    rt_err_t (*tp_move_updn)(void *param);

    rt_err_t (*tp_move_up)(void *param);
    rt_err_t (*tp_touch_up)(void *param);
};

struct app_main_data_t
{
    rt_display_data_t disp;
    rt_event_t        event;
    rt_mq_t           mq;
    uint8_t           bl;
    uint8_t           bl_en;
    uint8_t           bl_time;
    uint8_t           pm_status;

    rt_timer_t        clk_timer;
#if APP_TIMING_LIGHTOFF
    rt_timer_t        bl_timer;
#endif
    struct tm         *tmr_data;
    void (*timer_cb)(void);
    uint32_t          timer_cycle;
    rt_timer_t        job_timer;

    rt_slist_t        design_list;
    refresh_cb_t      refr[3];

    rt_int8_t         ver_page;
    rt_int8_t         clock_style;

    rt_uint8_t        tb_flag;
    rt_uint8_t        dir_mode;
    rt_int16_t        xdir;
    rt_int16_t        ydir;
    rt_int16_t        xoffset;
    rt_int16_t        yoffset;
    rt_int16_t        mov_speed;
    rt_uint8_t        smooth_design;
    rt_uint32_t       down_timestamp;
    rt_uint32_t       touch_event;

    struct rt_touchpanel_block touch_block;
    struct rt_touch_data down_point[1];
    struct rt_touch_data pre_point[1];
    struct rt_touch_data cur_point[1];

    struct app_touch_cb_t touch_cb;
#if APP_WDT_ENABLE
    rt_device_t wdt_dev;
#endif

    player_state_t play_state;
    rt_int8_t play_mode;
    rt_int8_t play_vol;

    rt_sem_t wait_sem;
    rt_uint8_t wait_msc;
};
extern struct app_main_data_t *app_main_data;
extern struct app_info *g_app_info;

/* Image source */
extern image_info_t img_whatsapp_160x160_info;
extern image_info_t img_whatsapp_info;
extern image_info_t img_clk0_hour_info;
extern image_info_t img_clk0_min_info;
extern image_info_t img_clk0_sec_info;
extern image_info_t img_clk0_center_info;

extern image_info_t img_clk1_hour_info;
extern image_info_t img_clk1_min_info;
extern image_info_t img_clk1_sec_info;

extern image_info_t img_clk2_hour_info;
extern image_info_t img_clk2_min_info;
extern image_info_t img_clk2_sec_info;

extern image_info_t img_heart_info;
/* Image source end */

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

rt_err_t app_load_img(img_load_info_t *info, rt_uint8_t *pbuf, rt_uint16_t w, rt_uint16_t h, rt_uint16_t offset, rt_uint8_t bitsize);

void app_main_timer_cb_register(void (*cb)(void), uint32_t ms);

void app_main_timer_cb_unregister(void);

/**
 * Design request.
 */
void app_design_request(rt_uint8_t urgent, design_cb_t *design, void *param);

/**
 * Refresh callback register.
 */
void app_refresh_register(rt_uint8_t winid, void *cb, void *param);

/**
 * Refresh callback unregister.
 */
void app_refresh_unregister(rt_uint8_t winid);

/**
 * Get refresh callback.
 */
void app_get_refresh_callback(rt_uint8_t winid, void **cb, void **param);
/**
 * Refresh callback request.
 */
void app_refresh_request(void *param);

/**
 * Direct refrest to LCD.
 */
rt_uint32_t app_str2num(const char *str, uint8_t len);
void app_main_touch_value_reset(void);
void app_main_touch_register(struct app_touch_cb_t *tcb);
void app_main_touch_unregister(void);
void app_main_register_timeout_cb(rt_err_t (*cb)(void *param), void *param, uint32_t ms);
void app_main_unregister_timeout_cb(void);
void app_main_unregister_timeout_cb_if_is(rt_err_t (*cb)(void *param));
void app_registe_refresh_done_cb(rt_err_t (*cb)(void *param), void *param);
rt_err_t app_main_touch_smooth_design(void *param);
rt_err_t app_main_touch_process(struct rt_touch_data *point, rt_uint8_t num);
void app_main_keep_screen_on(void);
void app_main_set_bl_timeout(uint32_t set_time);
void app_slide_refresh(page_refrsh_request_param_t *param);
void app_slide_refresh_undo(void);
void app_enter_page(struct app_page_data_t *page);
void app_update_page(struct app_page_data_t *page);
void app_main_touch_skip(rt_uint8_t event);
void app_main_set_time(struct tm *time);
void app_main_get_time(struct tm **time);

/**********************
 * SUB INCLUDE
 **********************/
#include <littlevgl2rtt.h>
#include <lvgl/lvgl.h>

#include "app_main_page.h"
#include "app_clock.h"
#include "app_dialog.h"
#include "app_weather.h"
#include "app_music.h"
#include "app_qrcode_page.h"
#include "app_message.h"
#include "app_alpha_win.h"
#include "app_lvgl_design.h"
#include "app_charging.h"
#include "app_funclist.h"
#include "app_asr.h"
#include "app_play.h"
#include "app_preview.h"
#include "app_file.h"
#include "auto_play.h"
#include "func_group.h"
#include "func_common.h"
#include "func_heartrate.h"
#include "func_setting.h"
#include "func_motion.h"
#include "func_backlight.h"
#include "func_time.h"
#include "setting_group.h"
#include "setting_common.h"

#endif
