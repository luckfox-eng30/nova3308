/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __OLPC_AP_H__
#define __OLPC_AP_H__

#include <rtthread.h>
#include "hal_def.h"

/* AP command */
#define FILE_INFO_REQ           0x41500002U //"AP"02
#define FIILE_READ_REQ          0x41500003U //"AP"03

/* AP return status */
#define AP_COMMAND_ACK          0x4141434BU //"AACK"
#define AP_STATUS_OK            0x0U
#define AP_STATUS_ERROR         0x1U

/**
 * file read request param
 */
#define FILE_READ_REQ_NAME_MAX_LEN  32
typedef struct _FILE_READ_REQ_PARAM
{
    /* request param*/
    char name[FILE_READ_REQ_NAME_MAX_LEN]; /* name buffer */
    rt_uint8_t  *buf;           /* buffer address, load file data to a buffer*/
    rt_uint32_t offset;         /* file offset */
    rt_uint32_t reqlen;         /* need len */

    /* return param*/
    rt_uint32_t totalsize;      /* file total size */
    rt_uint32_t rdlen;          /* read len */

} HAL_CACHELINE_ALIGNED FILE_READ_REQ_PARAM;

/**
 * Send command with AP.
 */
rt_err_t olpc_ap_command(rt_uint32_t cmd, void *param, rt_uint32_t paramlen);

#endif
