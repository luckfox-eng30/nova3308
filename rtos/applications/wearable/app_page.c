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

#define AUTO_RESIZE_ENABLE      1

struct page_cfg
{
    struct app_page_data_t *page;
    struct rt_display_config *wincfg;
};

const uint8_t format2depth[RTGRAPHIC_PIXEL_FORMAT_ARGB565 + 1] =
{
    [RTGRAPHIC_PIXEL_FORMAT_RGB332]     = 1,
    [RTGRAPHIC_PIXEL_FORMAT_RGB444]     = 2,
    [RTGRAPHIC_PIXEL_FORMAT_RGB565]     = 2,
    [RTGRAPHIC_PIXEL_FORMAT_RGB565P]    = 2,
    [RTGRAPHIC_PIXEL_FORMAT_RGB666]     = 3,
    [RTGRAPHIC_PIXEL_FORMAT_RGB888]     = 3,
    [RTGRAPHIC_PIXEL_FORMAT_ARGB888]    = 4,
    [RTGRAPHIC_PIXEL_FORMAT_ABGR888]    = 4,
    [RTGRAPHIC_PIXEL_FORMAT_ARGB565]    = 3,
};

extern rt_err_t app_main_refresh_request(struct rt_display_mq_t *disp_mq, rt_int32_t wait);
rt_err_t app_page_refresh(struct app_page_data_t *page, uint8_t page_num, uint8_t auto_resize)
{
    struct rt_device_graphic_info *info = &app_main_data->disp->info;
    struct rt_display_mq_t disp_mq;
    struct rt_display_config *wincfg;
#if AUTO_RESIZE_ENABLE
    struct page_cfg page_grp[3] = {{NULL, NULL}};
#endif
    int32_t start_x, start_y;
    uint16_t x, y;
    uint16_t w, h;

    if (page_num > 3 || page_num == 0)
        return -RT_ERROR;

    rt_memset(&disp_mq, 0, sizeof(struct rt_display_mq_t));
    for (int i = 0; i < page_num; i++)
    {
        start_x = page->hor_offset;
        start_y = page->ver_offset;
        x = 0;
        y = 0;
        w = page->w;
        h = page->h;

        wincfg = &disp_mq.win[page->win_id];

        if (page->hor_offset < 0)
        {
            if (page->win_loop)
            {
                start_x = page->hor_offset + page->vir_w;
            }
            else
            {
                start_x = 0;
                if (page->exit_side == EXIT_SIDE_LEFT)
                {
                    if (page->hor_offset < -info->width)
                        page->hor_offset = -info->width;
                    x = -page->hor_offset;
                    w = info->width - x;
                }
            }
        }
        if ((page->hor_offset + info->width) > page->vir_w)
        {
            if (!page->win_loop)
            {
                if (page->exit_side == EXIT_SIDE_RIGHT)
                {
                    w = info->width - ((page->hor_offset + info->width) % page->vir_w);
                    start_x = page->hor_offset;
                }
                else
                {
                    start_x = page->vir_w - info->width;
                }
            }
        }

        if (page->ver_offset < 0)
        {
            start_y = 0;
            if (page->exit_side == EXIT_SIDE_TOP)
            {
                if (page->ver_offset < -info->height)
                    page->ver_offset = -info->height;
                y = -page->ver_offset;
                if ((page->h + y) > info->height)
                    h = info->height - y;
                else
                    h = page->h;
            }
        }
        if ((page->ver_offset + MIN(page->h, info->height)) > page->h)
        {
            if (page->exit_side == EXIT_SIDE_BOTTOM)
            {
                h = info->height - ((page->ver_offset + info->height) % page->h);
                start_y = page->ver_offset;
            }
            else
            {
                start_y = MIN((page->h - info->height), page->ver_offset);
            }
        }

        wincfg->winId   = page->win_id;
        wincfg->zpos    = page->win_layer;
        wincfg->colorkey = 0;
        wincfg->alphaEn = 0;
        wincfg->format  = page->format;
        wincfg->lut     = page->lut;
        wincfg->lutsize = page->lutsize;
        wincfg->new_lut = page->new_lut;
        wincfg->hide_win = page->hide_win;
        wincfg->fb    = page->fb    + (start_x + start_y * page->vir_w) * format2depth[wincfg->format];
        wincfg->fblen = page->fblen - (start_x + start_y * page->vir_w) * format2depth[wincfg->format];
        wincfg->xVir  = page->vir_w;
        wincfg->w     = MIN(w, info->width);
        wincfg->h     = MIN(h, info->height);
        wincfg->x     = x + ((info->width  > page->w) ? (info->width  - page->w) / 2 : 0);
        wincfg->y     = y /*+ ((info->height > page->h) ? (info->height - page->h) / 2 : 0)*/;
        wincfg->ylast = wincfg->y;

        if (wincfg->h > 0)
        {
            disp_mq.cfgsta |= (1 << page->win_id);
#if AUTO_RESIZE_ENABLE
            page_grp[page->win_layer].page = page;
            page_grp[page->win_layer].wincfg = wincfg;
#endif
        }

        // rt_kprintf("%d %d %d %d %p %d %d %d %d %d %d\n",
        //            wincfg->winId, wincfg->zpos, start_x, start_y, wincfg->fb, wincfg->xVir,
        //            wincfg->w, wincfg->h, wincfg->x, wincfg->y,
        //            page->hor_offset);

        page = page->next;
    }

#if AUTO_RESIZE_ENABLE
    if (page_num > 1 && auto_resize)
    {
        for (int i = WIN_TOP_LAYER; i > WIN_BOTTOM_LAYER; i--)
        {
            if (page_grp[i].page == NULL || page_grp[i].page->format >= RTGRAPHIC_PIXEL_FORMAT_ARGB888)
                continue;

            for (int j = i - 1; j >= WIN_BOTTOM_LAYER; j--)
            {
                if (page_grp[j].page == NULL)
                    continue;

                start_x = start_y = 0;

                switch (page_grp[i].page->exit_side)
                {
                case EXIT_SIDE_TOP:
                    if ((page_grp[j].wincfg->y + page_grp[j].wincfg->h) > page_grp[i].wincfg->y)
                        page_grp[j].wincfg->h = page_grp[i].wincfg->y - page_grp[j].wincfg->y;
                    break;
                case EXIT_SIDE_BOTTOM:
                    if ((page_grp[i].wincfg->y + page_grp[i].wincfg->h) > page_grp[j].wincfg->y)
                    {
                        start_y = page_grp[i].wincfg->y + page_grp[i].wincfg->h - page_grp[j].wincfg->y;
                        page_grp[j].wincfg->y += start_y;
                        page_grp[j].wincfg->h -= start_y;
                    }
                    break;
                case EXIT_SIDE_RIGHT:
                    if ((page_grp[i].wincfg->x + page_grp[i].wincfg->w) > page_grp[j].wincfg->x)
                    {
                        start_x = page_grp[i].wincfg->x + page_grp[i].wincfg->w - page_grp[j].wincfg->x;
                        page_grp[j].wincfg->x += start_x;
                        page_grp[j].wincfg->w -= start_x;
                    }
                    break;
                case EXIT_SIDE_LEFT:
                    if ((page_grp[j].wincfg->x + page_grp[j].wincfg->w) > page_grp[i].wincfg->x)
                        page_grp[j].wincfg->w = page_grp[i].wincfg->x - page_grp[j].wincfg->x;
                    break;
                default:
                    break;
                }

                if (page_grp[j].wincfg->h <= 2 || page_grp[j].wincfg->w <= 2)
                {
                    disp_mq.cfgsta &= ~(1 << page_grp[j].page->win_id);
                    page_grp[j].wincfg = NULL;
                    page_grp[j].page = NULL;
                    continue;
                }

                if (start_x || start_y)
                    page_grp[j].wincfg->fb += (start_x + start_y * page_grp[j].page->vir_w) * format2depth[page_grp[j].wincfg->format];
            }
        }
    }
#endif

    if (disp_mq.cfgsta)
        app_main_refresh_request(&disp_mq, RT_WAITING_FOREVER);

    return RT_EOK;
}
