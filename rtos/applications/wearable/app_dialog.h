/**
  * Copyright (c) 2021 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_DIALOG__
#define __APP_DIALOG__
#include <rtthread.h>

enum dialog_type
{
    DIALOG_TEXT_OK_CANCEL,
    DIALOG_IMG_OK_CANCEL,
    DIALOG_TEXT_OK_ONLY,
    DIALOG_IMG_OK_ONLY,
    DIALOG_TEXT_NO_CHECK,
    DIALOG_IMG_NO_CHECK,
};

enum dialog_cb_type
{
    DIALOG_CALLBACK_NONE,
    DIALOG_CALLBACK_OK,
    DIALOG_CALLBACK_CANCEL,
};

struct dialog_desc
{
    char *title;
    char *text;
    img_load_info_t img;
    enum dialog_type type;
};

int app_dialog_check(void);
int app_dialog_exit(enum dialog_cb_type type);
int app_dialog_enter(struct app_page_data_t *p_page, struct dialog_desc *desc,
                     int refr_now, void (*cb)(enum dialog_cb_type type));
void app_dialog_init(void);

#endif
