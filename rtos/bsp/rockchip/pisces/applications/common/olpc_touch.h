/**************************************************************
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd
 * VENDOR_EDIT
 * File             : olpc_touch.h
 * Description      : Source file for touch function
 * Version          : 1.0
 * Date             : 2019-08-01
 * Author           : zhoufeng@swdp
 * ---------------- Revision History: --------------------------
 *   <version>        <date>                  < author >                                                        <desc>
 * Revision 1.1, 2019-08-01, zhoufeng@swdp, initial touch function
 ****************************************************************/

#ifndef __OLPC_TOUCH__
#define __OLPC_TOUCH__
#include <rtthread.h>

/**
 * Base structure of touch item
 */
struct olpc_touch_item
{
    struct olpc_touch_item *next;                       /**< next touch item */
    struct olpc_touch_item *prev;                       /**< prev touch item */
};

/**
 * Base structure of olpc touch
 */
enum olpc_touch_event
{
    TOUCH_EVENT_NULL = 0,
    TOUCH_EVENT_UP,
    TOUCH_EVENT_PROB,
    TOUCH_EVENT_SHORT_DOWN,
    TOUCH_EVENT_LONG_DOWN,
    TOUCH_EVENT_LONG_PRESS,
    TOUCH_EVENT_MOVE,
};

struct olpc_touch
{
    struct olpc_touch_item *touch_list;                 /**< used touch list */
    struct olpc_touch_item touch_header;                /**< touch list header */

    struct rt_semaphore     lock;                       /**< semaphore lock */
    enum   olpc_touch_event state;
    rt_tick_t               tick;
};

void olpc_touch_list_clear(void);
void olpc_touch_reset(void);
rt_err_t register_touch_item(struct olpc_touch_item *item,
                             void (*entry)(void *parameter),
                             void *parameter,
                             rt_int32_t touch_id);
rt_err_t unregister_touch_item(struct olpc_touch_item *item);

void update_item_coord(struct olpc_touch_item *item, rt_uint32_t fb_x, rt_uint32_t fb_y,
                       rt_uint32_t x_offset, rt_uint32_t y_offset);

#endif
