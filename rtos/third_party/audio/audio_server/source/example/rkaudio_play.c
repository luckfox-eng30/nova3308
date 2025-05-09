/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "audio_server.h"

#ifdef __LINUX__

#define PCM_DEVICE { \
    .open = playback_device_open_impl, \
    .start = playback_device_start_impl, \
    .write = playback_device_write_impl, \
    .stop = playback_device_stop_impl, \
    .abort = playback_device_abort_impl, \
    .close = playback_device_close_impl, \
}

void player_callback_test(player_handle_t self, play_info_t info, void *userdata);

static int playback_end = 0;
static player_handle_t player_test = NULL;
static play_cfg_t *cfg_test = NULL;
static player_cfg_t player_cfg =
{
    .preprocess_buf_size = 1024 * 40,
    .decode_buf_size = 1024 * 20,
    .preprocess_stack_size = 2048,
    .decoder_stack_size = 1024 * 12,
    .playback_stack_size = 2048,
    .tag = "one",
    .device = PCM_DEVICE,
    .listen = player_callback_test,
};

void player_callback_test(player_handle_t self, play_info_t info, void *userdata)
{
    printf("Playback end\n");
    playback_end = 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return -1;

    player_init();
    player_register_mp3dec();
    player_list_decoder();
    cfg_test = malloc(sizeof(play_cfg_t));
    cfg_test->target = argv[1];//malloc(1024);
    // memset(cfg_test->target, 0x0, 1024);
    player_cfg.name = malloc(10);
    sprintf(player_cfg.name, "out.pcm");
    player_cfg.resample_rate = 0;
    player_cfg.out_ch = 2;
    player_cfg.diff_out = 0;

    player_test = player_create(&player_cfg);
    printf("Create player %p\n", player_test);
    playback_set_volume(100);

    cfg_test->start_time = 0;
    cfg_test->preprocessor = (play_preprocessor_t)DEFAULT_FILE_PREPROCESSOR;
    cfg_test->freq_t = PLAY_FREQ_LOCALPLAY;
    cfg_test->need_free = 1;
    cfg_test->info_only = 0;
    playback_end = 0;

    printf("%s:now play %s start from %ld\n\n", __func__, cfg_test->target, cfg_test->start_time);
    player_play(player_test, cfg_test);

    while (playback_end == 0)
    {
        sleep(1);
    }

    player_stop(player_test);
    free(cfg_test->target);
    free(cfg_test);
    cfg_test = NULL;
    free(player_cfg.name);
    player_cfg.name = NULL;
    player_destroy(player_test);
    player_test = NULL;
    player_deinit();
}
#endif

