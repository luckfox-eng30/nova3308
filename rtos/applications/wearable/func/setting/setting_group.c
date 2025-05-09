/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include "app_main.h"

struct app_func app_setting_group[APP_SETTING_COMMON + 1] = {NULL};

void app_setting_set(enum app_setting_e index, struct app_func *ops)
{
    app_setting_group[index].init = ops->init;
    app_setting_group[index].init_param = ops->init_param;
    app_setting_group[index].enter = ops->enter;
    app_setting_group[index].param = ops->param;
    app_setting_group[index].exit = ops->exit;
    app_setting_group[index].pause = ops->pause;
    app_setting_group[index].resume = ops->resume;
    app_setting_group[index].pre_view = ops->pre_view;
}

void app_setting_pause(enum app_setting_e index)
{
    if (app_setting_group[index].paused == 1)
        return;

    app_setting_group[index].paused = 1;
    if (app_setting_group[index].pause)
    {
        app_setting_group[index].pause();
    }
}

void app_setting_resume(enum app_setting_e index)
{
    if (app_setting_group[index].paused == 0)
        return;

    app_setting_group[index].paused = 0;
    if (app_setting_group[index].resume)
    {
        app_setting_group[index].resume();
    }
}

void app_setting_exit(enum app_setting_e index)
{
    struct app_page_data_t *page = g_setting_page;
    struct app_setting_private *pdata = page->private;

    if (app_setting_group[index].exit)
    {
        app_setting_group[index].exit();
    }
    else if (app_setting_group[APP_SETTING_COMMON].exit)
    {
        app_setting_group[APP_SETTING_COMMON].exit();
    }
    pdata->setting_id = APP_SETTING_COMMON;
}

void app_setting_enter(enum app_setting_e index)
{
    struct app_page_data_t *page = g_setting_page;
    struct app_setting_private *pdata = page->private;

    app_setting_group[index].paused = 0;
    if (app_setting_group[index].enter)
    {
        app_setting_group[index].enter(app_setting_group[index].param);
        pdata->setting_id = index;
    }
    else if (app_setting_group[APP_SETTING_COMMON].enter)
    {
        app_setting_group[APP_SETTING_COMMON].enter(app_setting_group[APP_SETTING_COMMON].param);
        pdata->setting_id = APP_SETTING_COMMON;
    }
}

void app_setting_init(enum app_setting_e index)
{
    if (app_setting_group[index].init)
    {
        app_setting_group[index].init(app_setting_group[index].init_param);
    }
    else if (app_setting_group[APP_SETTING_COMMON].init)
    {
        app_setting_group[APP_SETTING_COMMON].init(app_setting_group[APP_SETTING_COMMON].init_param);
        app_setting_group[index].pre_view = app_setting_group[APP_SETTING_COMMON].pre_view;
    }
}

