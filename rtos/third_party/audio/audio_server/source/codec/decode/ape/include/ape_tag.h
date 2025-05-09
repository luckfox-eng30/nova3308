/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronic Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-03-12     Jair Wu      First version
 *
 */

#ifndef __APE_TAG_H
#define __APE_TAG_H

#include "play_ape.h"

int ape_read_header(APEDec *dec, struct play_ape *ape);
int ape_release_header(APEDec *dec);

#endif
