/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(RT_USING_OLPC_DEMO)

#include "drv_heap.h"
#include "image_info.h"
#include "olpc_display.h"
#include "olpc_ap.h"

#if defined(RT_USING_MODULE)
#include "dlmodule.h"
#endif

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
rt_event_t olpc_main_event;
RTM_EXPORT(olpc_main_event);

/*
 **************************************************************************************************
 *
 * olpc static functions
 *
 **************************************************************************************************
 */
#if defined(OLPC_DLMODULE_ENABLE)
/**
 * dlmodule request hook.
 */
static rt_uint8_t *olpc_dlmodule_request(const char *name)
{
    rt_err_t ret;
    FILE_READ_REQ_PARAM Param;
    FILE_READ_REQ_PARAM *pParam = &Param;

    RT_ASSERT(rt_strlen(name) < FILE_READ_REQ_NAME_MAX_LEN);

    rt_memset(pParam, 0, sizeof(FILE_READ_REQ_PARAM));

    /* Get dlmodule file size info */
    rt_strncpy(pParam->name, name, rt_strlen(name));
    ret = olpc_ap_command(FILE_INFO_REQ, pParam, sizeof(FILE_READ_REQ_PARAM));
    RT_ASSERT(ret == RT_EOK);

    /* Malloc buffer for dlmodule file */
    pParam->buf = (rt_uint8_t *)rt_dma_malloc_large(pParam->totalsize);
    RT_ASSERT(pParam->buf != RT_NULL);

    /* Read dlmodule file data */
    ret = olpc_ap_command(FIILE_READ_REQ, pParam, sizeof(FILE_READ_REQ_PARAM));
    RT_ASSERT(ret == RT_EOK);

    return pParam->buf;
}

/**
 * dlmodule release hook.
 */
static rt_err_t olpc_dlmodule_release(rt_uint8_t *param)
{
    rt_dma_free_large(param);

    return RT_EOK;
}

/**
 * Get dlmodule from & start dlmodule.
 */
int olpc_dlmodule_exec(const char *pgname)
{
    struct rt_dlmodule *module;
    struct rt_dlmodule_ops ops;
    ops.load = olpc_dlmodule_request;
    ops.unload = olpc_dlmodule_release;
    module = dlmodule_exec_custom(pgname, pgname, rt_strlen(pgname), &ops);
    if (module)
    {
        return RT_EOK;
    }
    return RT_ERROR;
}

int olpc_clock_app_init()
{
    return olpc_dlmodule_exec("clock.mo");
}
int olpc_block_app_init()
{
    return olpc_dlmodule_exec("block.mo");
}
int olpc_ebook_app_init()
{
    return olpc_dlmodule_exec("ebook.mo");
}
int olpc_snake_app_init()
{
    return olpc_dlmodule_exec("snake.mo");
}
int olpc_lyric_app_init()
{
    return olpc_dlmodule_exec("lyric.mo");
}
int olpc_note_app_init()
{
    return olpc_dlmodule_exec("note.mo");
}
int olpc_xscreen_app_init()
{
    return olpc_dlmodule_exec("xscreen.mo");
}
int olpc_jupiter_app_init()
{
    return olpc_dlmodule_exec("jupiter.mo");
}

#endif

/**
 * olpc_demo app init.
 */
static void olpc_apps_init(void)
{
#if defined(OLPC_APP_CLOCK_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_CLOCK);
#elif defined(OLPC_APP_EBOOK_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_EBOOK);
#elif defined(OLPC_APP_BLOCK_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_BLOCK);
#elif defined(OLPC_APP_SNAKE_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_SNAKE);
#elif defined(OLPC_APP_NOTE_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_NOTE);
#elif defined(OLPC_APP_XSCREEN_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_XSCREEN);
#elif defined(OLPC_APP_JUPITER_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_JUPITER);
#elif defined(OLPC_APP_BLN_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_BLN);
#elif defined(OLPC_APP_LYRIC_ENABLE)
    rt_event_send(olpc_main_event, EVENT_APP_LYRIC);
#endif
}

/*
 **************************************************************************************************
 *
 * olpc main thread
 *
 **************************************************************************************************
 */

/**
 * olpc main thread.
 */
static void olpc_main_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;

    olpc_main_event = rt_event_create("olpcmain_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_main_event != RT_NULL);
    olpc_apps_init();

    while (1)
    {
        ret = rt_event_recv(olpc_main_event,
                            EVENT_APP_CLOCK   | EVENT_APP_EBOOK   |
                            EVENT_APP_BLOCK   | EVENT_APP_SNAKE   |
                            EVENT_APP_NOTE    | EVENT_APP_XSCREEN |
                            EVENT_APP_BLN     | EVENT_APP_LYRIC   |
                            EVENT_APP_JUPITER,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        RT_ASSERT(ret == RT_EOK);

        if (event & EVENT_APP_CLOCK)
        {
#if defined(OLPC_APP_CLOCK_ENABLE)
            ret = olpc_clock_app_init();
#endif
        }
        else if (event & EVENT_APP_EBOOK)
        {
#if defined(OLPC_APP_EBOOK_ENABLE)
            ret = olpc_ebook_app_init();
#endif
        }
        else if (event & EVENT_APP_BLOCK)
        {
#if defined(OLPC_APP_BLOCK_ENABLE)
            ret = olpc_block_app_init();
#endif
        }
        else if (event & EVENT_APP_SNAKE)
        {
#if defined(OLPC_APP_SNAKE_ENABLE)
            ret = olpc_snake_app_init();
#endif
        }
        else if (event & EVENT_APP_NOTE)
        {
#if defined(OLPC_APP_NOTE_ENABLE)
            ret = olpc_note_app_init();
#endif
        }
        else if (event & EVENT_APP_BLN)
        {
#if defined(OLPC_APP_BLN_ENABLE)
            ret = olpc_bln_app_init();
#endif
        }
        else if (event & EVENT_APP_LYRIC)
        {
#if defined(OLPC_APP_LYRIC_ENABLE)
            ret = olpc_lyric_app_init();
#endif
        }
        else if (event & EVENT_APP_XSCREEN)
        {
#if defined(OLPC_APP_XSCREEN_ENABLE)
            ret = olpc_xscreen_app_init();
#endif
        }
        else if (event & EVENT_APP_JUPITER)
        {
#if defined(OLPC_APP_JUPITER_ENABLE)
            ret = olpc_jupiter_app_init();
#endif
        }

        if (ret)
        {
            rt_kprintf("olpc_start_app error: 0x%2x\n", event);
            olpc_apps_init();
        }
    }

    /* Thread deinit */

    rt_event_delete(olpc_main_event);
    olpc_main_event = RT_NULL;
}

/**
 * olpc clock demo application init.
 */
int olpc_main_init(void)
{
    rt_thread_t rtt_olpcmain;

    rtt_olpcmain = rt_thread_create("olpcmain", olpc_main_thread, RT_NULL, 2048, 10, 10);
    RT_ASSERT(rtt_olpcmain != RT_NULL);
    rt_thread_startup(rtt_olpcmain);

    return RT_EOK;
}

INIT_APP_EXPORT(olpc_main_init);
#endif
