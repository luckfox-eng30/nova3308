/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_MESSAGE_MAIN__
#define __APP_MESSAGE_MAIN__
#include <rtthread.h>

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */

/*
 **************************************************************************************************
 *
 * Struct & data define
 *
 **************************************************************************************************
 */

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

struct app_msg_private
{
    uint8_t  *fb[2];
    uint8_t  buf_id;
    rt_uint32_t offset;
    rt_uint8_t *logo_buf;
    rt_uint8_t *minilogo_buf;

    rt_int8_t   msg_cnt;
};

extern struct app_page_data_t *g_message_page;
extern struct app_touch_cb_t app_message_main_touch_cb;

rt_err_t app_message_main_refresh(struct rt_display_config *wincfg, void *param);

void app_message_main_init(void);
rt_err_t app_message_page_new_message(void *param);
rt_err_t app_message_page_show_message(void *param);
void app_message_page_exit(void);
rt_err_t app_message_anim_continue(void);

#endif
