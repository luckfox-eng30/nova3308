/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#include <stdio.h>
#include <string.h>

#include "drv_dsp.h"
#include "drv_heap.h"
#include "hal_base.h"

#include "app_main.h"

static const char *notice_name[NOTICE_MAX] =
{
    NOTICE_PATH"/iamhere.mp3",
};

#define EVENT_STOP      (0x1U << 1)
#define EVENT_CALLBACK  (0x1U << 2)
#define EVENT_EXIT      (0x1U << 3)

void app_play_callback(player_handle_t self, play_info_t info, void *userdata);
static int app_stop = 0;
static struct rt_event event;
static void (*app_play_cb)(play_info_t info) = NULL;
static void (*app_play_usr_cb)(void) = NULL;
static player_handle_t g_player = NULL;
static play_cfg_t play_cfg;
static play_cfg_t notice_cfg;
static player_cfg_t player_cfg =
{
    .preprocess_buf_size = 1024 * 4,
    .decode_buf_size = 1024 * 4,
    .preprocess_stack_size = 2048,
    .decoder_stack_size =  2048,
    .playback_stack_size = 2048,
    .tag = "one",
    .device = DEFAULT_PLAYBACK_DEVICE,
    .listen = app_play_callback,
    .name = "aw8896p",
    .resample_rate = 0,
    .out_ch = 2,
    .diff_out = 1,
};

void app_play_callback_handle(void *arg)
{
    struct app_main_data_t *pdata = app_main_data;
    rt_uint32_t e;

    while (1)
    {
        rt_event_recv(&event, EVENT_CALLBACK,
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER, &e);

        if (e & EVENT_CALLBACK)
        {
            switch (pdata->play_mode)
            {
            case APP_PLAY_RANDOM:
                rt_kprintf("Play random\n");
                app_main_data->play_state = PLAYER_STATE_RUNNING;
                app_play_random();
                break;
            case APP_PLAY_LOOP:
                rt_kprintf("Play loop\n");
                app_main_data->play_state = PLAYER_STATE_RUNNING;
                player_play(g_player, &play_cfg);
                break;
            case APP_PLAY_LIST:
                rt_kprintf("Play list\n");
                app_main_data->play_state = PLAYER_STATE_RUNNING;
                app_play_next();
                break;
            default:
                break;
            }
        }
    }
}

void app_play_set_callback(void (*cb)(void))
{
    app_play_usr_cb = cb;
}

void app_play_music_callback(play_info_t info)
{
    if (info == PLAY_INFO_IDLE)
    {
        rt_event_send(&event, EVENT_CALLBACK);
    }
    else if (app_stop == 0)
    {
        app_main_data->play_state = PLAYER_STATE_IDLE;
        app_music_design_update();
    }
    app_stop = 0;
}

void app_play_notice_callback(play_info_t info)
{
    if (app_play_usr_cb)
        app_play_usr_cb();

    if (play_cfg.start_time != 0)
    {
        app_play_cb = app_play_music_callback;
        app_main_data->play_state = PLAYER_STATE_RUNNING;
        player_play(g_player, &play_cfg);
    }
    else
    {
        app_play_cb = NULL;
        app_main_data->play_state = PLAYER_STATE_IDLE;
    }
}

void app_play_callback(player_handle_t self, play_info_t info, void *userdata)
{
    if (app_play_cb)
        app_play_cb(info);
}

static void app_play_display_target(char *path)
{
    char *target;
    char *head, *tail;

    if (path == NULL)
    {
        app_music_name_design(NULL);
        return;
    }

    target = rt_malloc(MAX_FILE_NAME);
    rt_memset(target, 0, MAX_FILE_NAME);

    head = strrchr(path, '/');
    if (head == NULL)
        head = path;
    tail = strrchr(path, '.');
    if (tail == NULL)
        tail = path + strlen(path);
    rt_memcpy(target, head + 1, tail - 1 - head);
    app_music_name_design(target);
    rt_free(target);
}

void app_play_init(void)
{
    rt_thread_t callback_handle;

    memset(&play_cfg, 0, sizeof(play_cfg_t));

    player_init();
    player_register_mp3dec();
    // player_list_decoder();

    rt_event_init(&event, "playcb", RT_IPC_FLAG_FIFO);
    callback_handle = rt_thread_create("playcb",
                                       app_play_callback_handle,
                                       NULL,
                                       2048,
                                       20,
                                       10);
    if (!callback_handle)
        rt_kprintf("Create callback handle failed\n");
    else
        rt_thread_startup(callback_handle);

    g_player = player_create(&player_cfg);
    playback_set_volume(app_main_data->play_vol * 25);

    play_cfg.target = get_audio();
    app_play_display_target(play_cfg.target);
}

void app_play_notice(int index)
{
    uint32_t time;

    if (index < 0 && index >= NOTICE_MAX)
        return;

    app_play_cb = app_play_notice_callback;
    memset(&notice_cfg, 0, sizeof(play_cfg_t));
    notice_cfg.target = (char *)notice_name[index];
    notice_cfg.preprocessor = (play_preprocessor_t)DEFAULT_FILE_PREPROCESSOR;
    notice_cfg.need_free = 1;

    time = player_get_total_time(g_player) / 1000;
    play_cfg.start_time = player_get_cur_time(g_player) / 1000;
    if ((time - play_cfg.start_time) < 3)
        play_cfg.start_time = 0;

    app_main_data->play_state = PLAYER_STATE_RUNNING;
    player_play(g_player, &notice_cfg);
}

void app_play_music(char *target)
{
    int ret;

    app_play_cb = app_play_music_callback;

    if (target == NULL)
        target = get_audio();

    app_play_display_target(target);
    if (target == NULL)
    {
        app_main_data->play_state = PLAYER_STATE_IDLE;
        rt_kprintf("Target is NULL\n");
        return;
    }

    ret = access(target, F_OK);
    if (ret < 0)
    {
        app_main_data->play_state = PLAYER_STATE_IDLE;
        rt_kprintf("file not exist\n");
        return;
    }

    play_cfg.target = target;
    play_cfg.preprocessor = (play_preprocessor_t)DEFAULT_FILE_PREPROCESSOR;
    play_cfg.need_free = 1;
    if (app_main_data->play_state != PLAYER_STATE_IDLE)
        app_stop = 1;
    else
        app_main_data->play_state = PLAYER_STATE_RUNNING;
    player_play(g_player, &play_cfg);
    app_music_design_update();
}

void app_play_pause(void)
{
    player_state_t state = app_main_data->play_state;

    if (app_play_cb == app_play_notice_callback)
        return;

    switch (state)
    {
    case PLAYER_STATE_RUNNING:
        rt_kprintf("player pause\n");
        player_pause(g_player);
        app_main_data->play_state = player_get_state(g_player);
        app_music_design_update();
        break;
    case PLAYER_STATE_PAUSED:
        rt_kprintf("player resume\n");
        player_resume(g_player);
        app_main_data->play_state = player_get_state(g_player);
        app_music_design_update();
        break;
    default:
        rt_kprintf("player start\n");
        app_play_music(NULL);
        break;
    }
}

void app_play_stop(void)
{
    if (app_main_data->play_state != PLAYER_STATE_RUNNING)
        return;

    player_stop(g_player);
    app_stop = 1;
    app_main_data->play_state = PLAYER_STATE_IDLE;
    app_music_design_update();
}

void app_play_random(void)
{
    char *target;

    target = app_random_file();
    if (target == NULL)
        return;

    app_play_display_target(play_cfg.target);

    app_play_music(target);
}

void app_play_next(void)
{
    char *target;

    if (app_main_data->play_mode == APP_PLAY_RANDOM)
        target = app_random_file();
    else
        target = app_next_file();
    if (target == NULL)
    {
        app_main_data->play_state = PLAYER_STATE_IDLE;
        app_music_design_update();
    }

    app_play_display_target(play_cfg.target);

    if (app_main_data->play_state == PLAYER_STATE_RUNNING)
    {
        app_play_music(target);
    }
    else
    {
        player_stop(g_player);
        app_main_data->play_state = PLAYER_STATE_IDLE;
        app_music_design_update();
    }
}

void app_play_prev(void)
{
    char *target;

    if (app_main_data->play_mode == APP_PLAY_RANDOM)
        target = app_random_file();
    else
        target = app_prev_file();
    if (target == NULL)
    {
        app_main_data->play_state = PLAYER_STATE_IDLE;
        app_music_design_update();
    }

    app_play_display_target(play_cfg.target);

    if (app_main_data->play_state == PLAYER_STATE_RUNNING)
    {
        app_play_music(target);
    }
    else
    {
        player_stop(g_player);
        app_main_data->play_state = PLAYER_STATE_IDLE;
        app_music_design_update();
    }
}

void app_music_vol_switch(void)
{
    static int dir = 1;

    if ((app_main_data->play_vol + dir) > APP_PLAY_VOL_MAX)
        dir = -1;
    if ((app_main_data->play_vol + dir) < APP_PLAY_VOL_MIN)
        dir = 1;

    app_main_data->play_vol += dir;

    playback_set_volume(app_main_data->play_vol * 25);
    save_app_info(app_main_data);

    app_music_design_update();
}

void app_music_mode_switch(void)
{
    app_main_data->play_mode++;
    if (app_main_data->play_mode > APP_PLAY_RANDOM)
        app_main_data->play_mode = APP_PLAY_LIST;

    save_app_info(app_main_data);

    app_music_design_update();
}
