/**************************************************************
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd
 * VENDOR_EDIT
 * File             : olpc_touch.c
 * Description      : Source file for touch function
 * Version          : 1.0
 * Date             : 2019-08-01
 * Author           : zhoufeng@swdp
 * ---------------- Revision History: --------------------------
 *   <version>        <date>                  < author >                                                        <desc>
 * Revision 1.1, 2019-08-01, zhoufeng@swdp, initial touch function
 ****************************************************************/
#include <rtthread.h>
#if defined(RT_USING_OLPC_DEMO) && defined(RT_USING_PISCES_TOUCH)
#include <rtdevice.h>

#include <drv_touch.h>
#include <drv_touch_s3706.h>
#include "olpc_touch.h"

#include "image_info.h"

#define TOUCH_DOWN_PROB_TIME        ((RT_TICK_PER_SECOND / 1000) * 20)
#define TOUCH_LONG_DOWN_PROB_TIME   ((RT_TICK_PER_SECOND / 1000) * 1000)
#define TOUCH_LONG_PRESS_STEP_TIME  ((RT_TICK_PER_SECOND / 1000) * 200)

static struct olpc_touch g_olpc_touch =
{
    .state = TOUCH_EVENT_UP,
};

rt_err_t register_touch_item(struct olpc_touch_item *item,
                             void (*entry)(void *parameter),
                             void *parameter,
                             rt_int32_t touch_id)
{
    RT_ASSERT(item != RT_NULL);

    //check list
    {
        struct olpc_touch_item *head = g_olpc_touch.touch_list;

        head = head->next;
        while (head != g_olpc_touch.touch_list)
        {
            if (head == item)
            {
                return RT_EOK;
            }
            head = head->next;
        }
    }

    // register item
    {
        struct olpc_touch_item *touch_list = g_olpc_touch.touch_list;

        item->next = touch_list->next;
        item->next->prev = item;

        touch_list->next = item;
        item->prev = touch_list;

        image_info_t *image = (image_info_t *)item;
        image->touch_callback = (void *)entry;
        image->parameter = parameter;
        image->touch_id = touch_id;
        image->touch_state = TOUCH_EVENT_UP;
    }

    return RT_EOK;
}
RTM_EXPORT(register_touch_item);

rt_err_t unregister_touch_item(struct olpc_touch_item *item)
{
    RT_ASSERT(item != RT_NULL);

    struct olpc_touch_item *touch_list = g_olpc_touch.touch_list;

    touch_list = touch_list->next;
    while (touch_list != g_olpc_touch.touch_list)
    {
        if (touch_list == item)
        {
            touch_list->next->prev = item->prev;
            touch_list->prev->next = item->next;

            return RT_EOK;
        }
        touch_list = touch_list->next;
    }

    return RT_ERROR;
}
RTM_EXPORT(unregister_touch_item);

void update_item_coord(struct olpc_touch_item *item, rt_uint32_t fb_x, rt_uint32_t fb_y, rt_uint32_t x_offset, rt_uint32_t y_offset)
{
    RT_ASSERT(item != RT_NULL);

    image_info_t *image = (image_info_t *)item;

    image->x_scr = fb_x + x_offset;
    image->y_scr = fb_y + y_offset;

    //rt_kprintf("%s, x = %d, y = %d.\n", __func__, image->x_scr, image->y_scr);
}
RTM_EXPORT(update_item_coord);

rt_uint8_t is_tp_in_item(struct point_info *point, image_info_t *item)
{
    RT_ASSERT(point != RT_NULL);
    RT_ASSERT(item != RT_NULL);

    if ((item->x_scr < point->x) && (point->x < item->x_scr + item->w))
        if ((item->y_scr < point->y) && (point->y < item->y_scr + item->h))
        {
            return 1;
        }

    return 0;
}
RTM_EXPORT(is_tp_in_item);

void olpc_touch_reset(void)
{
    g_olpc_touch.state = TOUCH_EVENT_NULL;
}
RTM_EXPORT(olpc_touch_reset);

rt_err_t iterate_touch_item_list(struct olpc_touch_item *header, struct point_info *point)
{
    image_info_t *image = RT_NULL;
    rt_tick_t curtick = rt_tick_get();
    struct olpc_touch_item *touch_item;

    RT_ASSERT(header != RT_NULL);
    RT_ASSERT(point != RT_NULL);

    // touch up, clear the touch state & call touchup callback
    if (point->x == 0 && point->y == 0)
    {
        g_olpc_touch.state = TOUCH_EVENT_UP;
        g_olpc_touch.tick  = curtick;

        touch_item = header->next;
        while (touch_item != header)
        {
            image = (image_info_t *)touch_item;
            if (image->touch_state != TOUCH_EVENT_UP)
            {
                image->touch_state = TOUCH_EVENT_UP;
                image->touch_callback(image->touch_id, TOUCH_EVENT_UP, point, image->parameter);
            }
            touch_item = touch_item->next;
        }
        return RT_EOK;
    }

    // touch probe, check if touch occurs
    if (g_olpc_touch.state == TOUCH_EVENT_UP)
    {
        g_olpc_touch.state = TOUCH_EVENT_PROB;
        g_olpc_touch.tick = curtick;

        return RT_EOK;
    }
    // touch short down process
    else if (g_olpc_touch.state == TOUCH_EVENT_PROB)
    {
        if ((curtick - g_olpc_touch.tick) < TOUCH_DOWN_PROB_TIME)
        {
            return RT_EOK;
        }

        g_olpc_touch.state = TOUCH_EVENT_SHORT_DOWN;

        touch_item = header->next;
        while (touch_item != header)
        {
            image = (image_info_t *)touch_item;
            if (is_tp_in_item(point, image) != 0)
            {
                image->touch_state = TOUCH_EVENT_SHORT_DOWN;
                image->touch_callback(image->touch_id, TOUCH_EVENT_SHORT_DOWN, point, image->parameter);
            }
            touch_item = touch_item->next;
        }
    }
    // touch long down or move check
    else if (g_olpc_touch.state == TOUCH_EVENT_SHORT_DOWN)
    {
        touch_item = header->next;
        while (touch_item != header)
        {
            image = (image_info_t *)touch_item;

            if (image->touch_state == TOUCH_EVENT_SHORT_DOWN)
            {
                image->touch_callback(image->touch_id, TOUCH_EVENT_MOVE, point, image->parameter);

                if (is_tp_in_item(point, image) != 0)
                {
                    if ((curtick - g_olpc_touch.tick) >= TOUCH_LONG_DOWN_PROB_TIME)
                    {
                        g_olpc_touch.state = TOUCH_EVENT_LONG_DOWN;
                        image->touch_state = TOUCH_EVENT_LONG_DOWN;
                        image->touch_callback(image->touch_id, TOUCH_EVENT_LONG_DOWN, point, image->parameter);
                    }
                }
                else
                {
                    // if any shortdown item outof range, enter move state
                    g_olpc_touch.state = TOUCH_EVENT_MOVE;
                    break;
                }

            }
            touch_item = touch_item->next;
        }
    }
    // touch long press process
    else if (g_olpc_touch.state == TOUCH_EVENT_LONG_DOWN)
    {
        touch_item = header->next;
        while (touch_item != header)
        {
            image = (image_info_t *)touch_item;
            if (image->touch_state == TOUCH_EVENT_LONG_DOWN)
            {
                image->touch_callback(image->touch_id, TOUCH_EVENT_MOVE, point, image->parameter);

                if (is_tp_in_item(point, image) != 0)
                {
                    if ((curtick - g_olpc_touch.tick) >= TOUCH_LONG_DOWN_PROB_TIME + TOUCH_LONG_PRESS_STEP_TIME)
                    {
                        g_olpc_touch.tick += TOUCH_LONG_PRESS_STEP_TIME;
                        image->touch_callback(image->touch_id, TOUCH_EVENT_LONG_PRESS, point, image->parameter);
                    }
                }
                else
                {
                    // if any longdown item outof range, enter move state
                    g_olpc_touch.state = TOUCH_EVENT_MOVE;
                    break;
                }
            }
            touch_item = touch_item->next;
        }
    }
    // touch move process
    else if (g_olpc_touch.state == TOUCH_EVENT_MOVE)
    {
        touch_item = header->next;
        while (touch_item != header)
        {
            image = (image_info_t *)touch_item;
            if (image->touch_state != TOUCH_EVENT_UP)
            {
                image->touch_state = TOUCH_EVENT_MOVE;
                image->touch_callback(image->touch_id, TOUCH_EVENT_MOVE, point, image->parameter);
            }
            touch_item = touch_item->next;
        }
    }

    return RT_EOK;
}
RTM_EXPORT(iterate_touch_item_list);

static void touch_mgr_thread_entry(void *parameter)
{
    rt_err_t ret = RT_EOK;
    touch_device_t *touch_dev = RT_NULL;

    struct point_info point;
    struct olpc_touch_item *touch_list = (struct olpc_touch_item *)parameter;

    touch_dev = (touch_device_t *)rt_device_find("s3706");
    RT_ASSERT(touch_dev != RT_NULL);

    ret = rt_device_open((rt_device_t)touch_dev, RT_DEVICE_FLAG_RDWR);
    RT_ASSERT(ret == RT_EOK);

    while (1)
    {
        ret = rt_mq_recv(&touch_dev->tp_mq, &point, sizeof(struct point_info), RT_WAITING_FOREVER);
        RT_ASSERT(ret == RT_EOK);

        //rt_kprintf("tp mgr point: x = 0x%x, y = 0x%x, z = 0x%x.\n", point.x, point.y, point.z);

        /* iterate touch item list */
        iterate_touch_item_list(touch_list, &point);
    }

    rt_device_close((rt_device_t)touch_dev);
}

void olpc_touch_list_clear(void)
{
    struct olpc_touch_item *head = &g_olpc_touch.touch_header;

    head->next = head;
    head->prev = head;
    g_olpc_touch.touch_list = head;
    g_olpc_touch.state = TOUCH_EVENT_UP;//TOUCH_EVENT_NULL;
}
RTM_EXPORT(olpc_touch_list_clear);

int olpc_touch_init(void)
{
    struct olpc_touch_item *head = &g_olpc_touch.touch_header;

    head->next = head;
    head->prev = head;
    g_olpc_touch.touch_list = head;

    /* create touch manager thread */

    rt_thread_t touch_thread;
    touch_thread = rt_thread_create("tpmgr",
                                    touch_mgr_thread_entry, &g_olpc_touch.touch_header,
                                    1024, 10, 20);
    if (touch_thread != RT_NULL)
        rt_thread_startup(touch_thread);

    return RT_EOK;
}
INIT_APP_EXPORT(olpc_touch_init);

#endif
