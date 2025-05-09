/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_FUNCLIST_GROUP_H__
#define __APP_FUNCLIST_GROUP_H__

enum app_func_e
{
    // APP_FUNC_MESSAGE,           // message
    // APP_FUNC_WEATHER,           // weather
    // APP_FUNC_BUSCARD,           // bus_card
    // APP_FUNC_ALIPAY,            // alipay
    // APP_FUNC_STOPWATCH,         // stopwatch
    // APP_FUNC_TIMTER,            // clock_timer
    APP_FUNC_HEARTRATE,         // heart_rate
    APP_FUNC_EXERCISE,          // physical_exercise
    APP_FUNC_SETTING,           // system_menu
    APP_FUNC_BREATH,            // breathing_training
    APP_FUNC_BAROMETER,         // altitude_barometer
    APP_FUNC_COMPASS,           // compass
    APP_FUNC_PRESSURE,          // pressure
    // APP_FUNC_ALARM,             // alarm_clock
    // APP_FUNC_SLEEP,             // sleep
    // APP_FUNC_NODISTURB,         // no_disturb,
    // APP_FUNC_DISPLAY,           // screen_display
    // APP_FUNC_FLASHLIGHT,        // flashlight
    // APP_FUNC_FINDPHONE,         // find_phone
    APP_FUNC_ACTIVITY,          // activity_record
    APP_FUNC_EXERCISE_RECORD,   // exercise_record
    APP_FUNC_TRAINING_STATUS,   // training_status
    APP_FUNC_GUIDE,             // user_guide
    APP_FUNC_ABOUT,             // about_watche
    APP_FUNC_COMMON,            // for all func
};

struct app_func
{
    void (*init)(void *param);
    void *init_param;
    void (*enter)(void *param);
    void *param;
    void (*exit)(void);
    void *pre_view;
    void (*pause)(void);
    void (*resume)(void);
    uint8_t paused;
};
extern struct app_func app_func_group[];

void app_func_init(enum app_func_e index);
void app_func_enter(enum app_func_e index);
void app_func_exit(enum app_func_e index);
void app_func_pause(enum app_func_e index);
void app_func_resume(enum app_func_e index);
void app_func_set_preview(enum app_func_e index, void *pre_view);
void *app_func_get_preview(enum app_func_e index);
void app_func_set(enum app_func_e index, struct app_func *ops);

#endif
