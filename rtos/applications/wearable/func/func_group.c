/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include "app_main.h"

struct app_func app_func_group[APP_FUNC_COMMON + 1] = {NULL};
static int music_pause;

void app_func_set(enum app_func_e index, struct app_func *ops)
{
    app_func_group[index].init = ops->init;
    app_func_group[index].init_param = ops->init_param;
    app_func_group[index].enter = ops->enter;
    app_func_group[index].param = ops->param;
    app_func_group[index].exit = ops->exit;
    app_func_group[index].pause = ops->pause;
    app_func_group[index].resume = ops->resume;
    app_func_group[index].pre_view = ops->pre_view;
}

void app_func_pause(enum app_func_e index)
{
    if (app_func_group[index].paused == 1)
        return;

    app_func_group[index].paused = 1;
    if (app_func_group[index].pause)
    {
        app_func_group[index].pause();
    }
}

void app_func_resume(enum app_func_e index)
{
    if (app_func_group[index].paused == 0)
        return;

    app_func_group[index].paused = 0;
    if (app_func_group[index].resume)
    {
        app_func_group[index].resume();
    }
}

void app_func_set_preview(enum app_func_e index, void *pre_view)
{
    app_func_group[index].pre_view = pre_view;
}

void *app_func_get_preview(enum app_func_e index)
{
    return app_func_group[index].pre_view;
}

void app_func_exit(enum app_func_e index)
{
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *pdata = page->private;

    if (app_func_group[index].exit)
    {
        app_func_group[index].exit();
    }
    else if (app_func_group[APP_FUNC_COMMON].exit)
    {
        app_func_group[APP_FUNC_COMMON].exit();
    }
    pdata->func_id = APP_FUNC_COMMON;

    if (music_pause)
    {
        music_pause = 0;
        app_play_pause();
    }
}

void app_func_enter(enum app_func_e index)
{
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *pdata = page->private;

    app_func_group[index].paused = 0;
    if (app_func_group[index].enter)
    {
        app_func_group[index].enter(app_func_group[index].param);
        pdata->func_id = index;
    }
    else if (app_func_group[APP_FUNC_COMMON].enter)
    {
        app_func_group[APP_FUNC_COMMON].enter(app_func_group[APP_FUNC_COMMON].param);
        pdata->func_id = APP_FUNC_COMMON;
    }
}

void app_func_init(enum app_func_e index)
{
    if (app_main_data->play_state == PLAYER_STATE_RUNNING)
    {
        music_pause = 1;
        app_play_pause();
    }
    else
    {
        music_pause = 0;
    }
    if (app_func_group[index].init)
    {
        app_func_group[index].init(app_func_group[index].init_param);
    }
    else if (app_func_group[APP_FUNC_COMMON].init)
    {
        app_func_group[APP_FUNC_COMMON].init(app_func_group[APP_FUNC_COMMON].init_param);
        app_func_group[index].pre_view = app_func_group[APP_FUNC_COMMON].pre_view;
    }
}

