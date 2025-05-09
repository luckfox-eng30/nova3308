/*
 * Copyright (c) 2020 Rockchip Electronic Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APP_FILE_H__
#define __APP_FILE_H__

#include "app_main.h"

#define ROOT_NO_MOUNTED    (0x1UL << 0)
#define SD0_NO_MOUNTED     (0x1UL << 1)

#define MAX_FILE_NAME       512

void get_app_info(struct app_main_data_t *info);
void save_app_info(struct app_main_data_t *info);
int rootfs_check(void);
void scan_audio(void);
char *get_audio(void);
char *app_random_file(void);
char *app_next_file(void);
char *app_prev_file(void);

#endif
