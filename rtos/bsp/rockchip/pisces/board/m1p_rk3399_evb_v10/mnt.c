/**
  * Copyright (c) 2019 Rockchip Electronic Co.,Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  ******************************************************************************
  * @file    mnt.c
  * @version V0.1
  * @brief   filesystem mount table
  *
  * Change Logs:
  * Date                Author          Notes
  * 2019-06-11     Cliff.Chen      first implementation
  *
  ******************************************************************************
  */

/** @addtogroup RKBSP_Board_Driver
 *  @{
 */

/** @addtogroup MNT
 *  @{
 */

/** @defgroup How_To_Use How To Use
 *  @{
 @verbatim

 ==============================================================================
                    #### How to use ####
 ==============================================================================
 This file provide mount table for filesystem. The patition defined in mount table will be mounted automatically.

 @endverbatim
 @} */

#include <rtthread.h>
#include <rtdevice.h>

#ifdef RT_USING_DFS_MNTTABLE
#include <dfs_fs.h>

#include "drv_flash_partition.h"

/********************* Private Variable Definition ***************************/
/** @defgroup MNT_Private_Variable Private Variable
 *  @{
 */

/**
 * @brief  Config flash partition
 * @attention The snor_parts must be terminated with NULL.
 */
struct rt_flash_partition flash_parts[] =
{
    /* root */
    {
        .name       = "root",
        .offset     = 0x0,
        .size       = (0x1000000),
        .mask_flags = PART_FLAG_RDWR,
    },

    /* end */
    {
        .name = RT_NULL,
    }
};

/**
 * @brief  Config mount table of filesystem
 * @attention The mount_table must be terminated with NULL, and the partition's name
 * must be the same as above.
 */
const struct dfs_mount_tbl mount_table[] =
{
    {"root", "/", "elm", 0, 0},
    {0}
};

/** @} */  // MNT_Private_Variable

/** @} */  // MNT

/** @} */  // RKBSP_Board_Driver

#endif
