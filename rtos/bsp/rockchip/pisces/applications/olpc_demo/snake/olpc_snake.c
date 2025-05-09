/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_SNAKE_ENABLE)

#include "drv_heap.h"
#include "applications/common/image_info.h"
#include "applications/common/olpc_display.h"

#if defined(RT_USING_PISCES_TOUCH)
#include "drv_touch.h"
#include "applications/common/olpc_touch.h"
#endif

/**
 * color palette for 1bpp
 */
static uint32_t bpp0_lut[2] =
{
    0x000000, 0x00FFFFFF
};

static uint32_t bpp1_lut[2] =
{
    //0x000000, 0x00FFFFFF
    0x000000, 0x0007E000    //00000 111111 00000
};

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
 * Game region:(23,40) (756,1113), snake 52x51
 * Next region:(797,432) (963, 599), snake 22x22
 *
 **************************************************************************************************
 */
#define SNAKE_GRAY0_WIN     0
#define SNAKE_GRAY1_WIN     1

#define CTRL0_REGION_X      0
#define CTRL0_REGION_Y      50
#define CTRL0_REGION_W      WIN_LAYERS_W
#define CTRL0_REGION_H      110
#define CTRL0_FB_W          960                           /* control frame buffer w */
#define CTRL0_FB_H          CTRL0_REGION_H                /* control frame buffer h */

#define SNAKE_REGION_X      0
#define SNAKE_REGION_Y      180
#define SNAKE_REGION_W      WIN_LAYERS_W
#define SNAKE_REGION_H      1560
#define SNAKE_FB_W          960                           /* snake frame buffer w */
#define SNAKE_FB_H          SNAKE_REGION_H                /* snake frame buffer h */

#define CTRL1_REGION_X      0
#define CTRL1_REGION_Y      1830
#define CTRL1_REGION_W      WIN_LAYERS_W
#define CTRL1_REGION_H      420
#define CTRL1_FB_W          (((420 + 31) / 32) * 32)      /* control frame buffer w */
#define CTRL1_FB_H          CTRL1_REGION_H                 /* control frame buffer h */

/* Event define */
#define EVENT_DISPLAY_REFRESH   (0x01UL << 0)
#define EVENT_GAME_PROCESS      (0x01UL << 1)
#define EVENT_GAME_EXIT         (0x01UL << 2)
#define EVENT_SRCSAVER_ENTER    (0x01UL << 3)
#define EVENT_SRCSAVER_EXIT     (0x01UL << 4)

/* Game command define */
#define CMD_GAME_SNAKE_MOVE     (0x01UL << 0)

/* Display command define */
#define CMD_UPDATE_GAME         (0x01UL << 0)
#define CMD_UPDATE_GAME_BK      (0x01UL << 1)
#define CMD_UPDATE_GAME_SNAKE   (0x01UL << 2)
#define CMD_UPDATE_GAME_OVER    (0x01UL << 3)

#define CMD_UPDATE_CTRL0        (0x01UL << 4)
#define CMD_UPDATE_CTRL0_SCORE  (0x01UL << 5)
#define CMD_UPDATE_CTRL0_EXIT   (0x01UL << 6)

#define CMD_UPDATE_CTRL1        (0x01UL << 7)
#define CMD_UPDATE_CTRL1_PLAY   (0x01UL << 8)
#define CMD_UPDATE_CTRL1_UP     (0x01UL << 9)
#define CMD_UPDATE_CTRL1_DOWN   (0x01UL << 10)
#define CMD_UPDATE_CTRL1_LEFT   (0x01UL << 11)
#define CMD_UPDATE_CTRL1_RIGHT  (0x01UL << 12)

#define SNAKE_SRCSAVER_TIME     (RT_TICK_PER_SECOND * 15)
/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern image_info_t snake_bkg_info;
extern image_info_t snake_gameover_info;
extern image_info_t snake_blkempty_info;
extern image_info_t snake_blkhalf_info;
extern image_info_t snake_blkfull_info;

extern image_info_t snake_num0_info;
extern image_info_t snake_num1_info;
extern image_info_t snake_num2_info;
extern image_info_t snake_num3_info;
extern image_info_t snake_num4_info;
extern image_info_t snake_num5_info;
extern image_info_t snake_num6_info;
extern image_info_t snake_num7_info;
extern image_info_t snake_num8_info;
extern image_info_t snake_num9_info;

extern image_info_t snake_play0_info;
extern image_info_t snake_pause0_info;
extern image_info_t snake_exit0_info;
extern image_info_t snake_exit1_info;
extern image_info_t snake_up0_info;
extern image_info_t snake_up1_info;
extern image_info_t snake_down0_info;
extern image_info_t snake_down1_info;
extern image_info_t snake_left0_info;
extern image_info_t snake_left1_info;
extern image_info_t snake_right0_info;
extern image_info_t snake_right1_info;

#if defined(OLPC_APP_SRCSAVER_ENABLE)
static void olpc_snake_screen_timer_start(void *parameter);
static void olpc_snake_screen_timer_stop(void *parameter);
#endif

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_snake_data
{
    rt_display_data_t disp;
    rt_timer_t    srctimer;

    rt_event_t  event;
    rt_uint32_t cmd;
    rt_uint32_t gamecmd;

    rt_uint8_t *snake_fb;
    rt_uint32_t snake_fblen;

    rt_uint8_t *ctrl0_fb;
    rt_uint32_t ctrl0_fblen;

    rt_uint8_t *ctrl1_fb;
    rt_uint32_t ctrl1_fblen;

    rt_uint8_t  btnupsta;
    rt_uint8_t  btndownsta;
    rt_uint8_t  btnleftsta;
    rt_uint8_t  btnrightsta;
    rt_uint8_t  btnstartsta;
    rt_uint8_t  btnexitsta;
    rt_uint8_t  btnpausesta;
};

static image_info_t *snake_nums[10] =
{
    &snake_num0_info,
    &snake_num1_info,
    &snake_num2_info,
    &snake_num3_info,
    &snake_num4_info,
    &snake_num5_info,
    &snake_num6_info,
    &snake_num7_info,
    &snake_num8_info,
    &snake_num9_info,
};

/*
 **************************************************************************************************
 *
 * snake game
 *
 **************************************************************************************************
 */
#define SNAKEGAME_SCOPE_COL             30
#define SNAKEGAME_SCOPE_ROW             50
#define SNAKEGAME_SNAKE_MAXFOOD         4
#define SNAKEGAME_SNAKE_MAXLEN          (SNAKEGAME_SCOPE_COL * SNAKEGAME_SCOPE_ROW)

#define SNAKEGAME_CELLTYPE_EMPTY        (1<<0)
#define SNAKEGAME_CELLTYPE_HALF         (1<<1)
#define SNAKEGAME_CELLTYPE_FULL         (1<<2)

#define SNAKELEFT                       0
#define SNAKERIGHT                      1
#define SNAKEUP                         2
#define SNAKEDOWN                       3

#define SNAKSTOP                        0
#define SNAKPLAY                        1
#define SNAKPAUSE                       2

#define SNAKETIMERDELAY                 500
#define SNAKETIMEDECSIZE                50

#define SNAKE_LINE_SCORE                100
#define SNAKE_LEVEL_STEP                (SNAKE_LINE_SCORE * 10)

struct tagSnakePoint
{
    rt_int8_t x;
    rt_int8_t y;
};

rt_uint8_t m_bsnakeGameover;
rt_uint8_t m_bsnakeState;
rt_int32_t m_snakeLevel;
rt_int32_t m_snakeLen;
rt_int32_t m_snakeCurDir;
rt_int32_t m_snakeCurDirTemp;
rt_int32_t m_snakeTotalScore;
rt_int8_t m_snakeArray[SNAKEGAME_SCOPE_ROW][SNAKEGAME_SCOPE_COL];
rt_int8_t m_snakeArraybk[SNAKEGAME_SCOPE_ROW][SNAKEGAME_SCOPE_COL];

struct tagSnakePoint m_ptFood[SNAKEGAME_SNAKE_MAXFOOD];
struct tagSnakePoint m_ptSnake[SNAKEGAME_SNAKE_MAXLEN];

rt_timer_t m_snakeTimerID = RT_NULL;

/*
 **************************************************************************************************
 *
 * 1bpp snake control API
 *
 **************************************************************************************************
 */

/**
 * clear snake array
 */
static void snake_array_clear(void)
{
    rt_uint16_t i, j;

    for (j = 0; j < SNAKEGAME_SCOPE_ROW; j++)
    {
        for (i = 0; i < SNAKEGAME_SCOPE_COL; i++)
        {
            if (j % 2 == 0)
            {
                if (i % 2 == 0)
                {
                    m_snakeArray[j][i] = SNAKEGAME_CELLTYPE_EMPTY;
                }
                else
                {
                    m_snakeArray[j][i] = SNAKEGAME_CELLTYPE_HALF;
                }
            }
            else
            {
                if (i % 2 == 0)
                {
                    m_snakeArray[j][i] = SNAKEGAME_CELLTYPE_HALF;
                }
                else
                {
                    m_snakeArray[j][i] = SNAKEGAME_CELLTYPE_EMPTY;
                }
            }
        }
    }
}

/**
 * clear backup snake array
 */
static void snake_arraybk_clear(void)
{
    rt_uint16_t i, j;

    for (j = 0; j < SNAKEGAME_SCOPE_ROW; j++)
    {
        for (i = 0; i < SNAKEGAME_SCOPE_COL; i++)
        {
            m_snakeArraybk[j][i] = 0xff;
        }
    }
}

/**
 * fill snake array
 */
static void snake_array_fill(void)
{
    rt_uint32_t i;
    rt_int8_t   col, row;

    for (i = 0; i < m_snakeLen; i++)
    {
        col = m_ptSnake[i].x;
        row = m_ptSnake[i].y;
        m_snakeArray[row][col] = SNAKEGAME_CELLTYPE_FULL;
    }

    for (i = 0; i < SNAKEGAME_SNAKE_MAXFOOD; i++)
    {
        col = m_ptFood[i].x;
        row = m_ptFood[i].y;
        if ((col != -1) && (row != -1))
        {
            m_snakeArray[row][col] = SNAKEGAME_CELLTYPE_FULL;
        }
    }
}

/**
 * check a point is overlapped snake body
 */
static rt_err_t snake_body_overlay_check(struct tagSnakePoint *pPoint)
{
    rt_uint16_t i;

    //for (i = 0; i < SNAKEGAME_SNAKE_MAXLEN; i++)
    for (i = 0; i < m_snakeLen; i++)
    {
        if ((pPoint->x == m_ptSnake[i].x) && (pPoint->y == m_ptSnake[i].y))
        {
            return RT_ERROR;
        }

        //if ((m_ptSnake[i].x == -1) || (m_ptSnake[i].y == -1))
        //{
        //    break;
        //}
    }

    return RT_EOK;
}

/**
 * create a snake point
 */
static rt_err_t snake_create_cell(struct tagSnakePoint *pPoint)
{
    rt_uint32_t len = 0;

    do
    {
        pPoint->x = rand() % SNAKEGAME_SCOPE_COL;
        pPoint->y = rand() % SNAKEGAME_SCOPE_ROW;
        if (++len > SNAKEGAME_SNAKE_MAXLEN)
        {
            return RT_ERROR;
        }
    }
    while (RT_EOK != snake_body_overlay_check(pPoint));

    return RT_EOK;
}

/**
 * clear snake list
 */
static void snake_bodylist_clear(void)
{
    rt_memset(m_ptSnake, -1, sizeof(struct tagSnakePoint) * SNAKEGAME_SNAKE_MAXLEN);
}

/**
 * initial snake list
 */
static void snake_bodylist_init(void)
{
    rt_memset(m_ptSnake, -1, sizeof(struct tagSnakePoint) * SNAKEGAME_SNAKE_MAXLEN);

    // init list body
    m_ptSnake[1].x = 0;
    m_ptSnake[1].y = 0;
    m_ptSnake[0].x = 1;
    m_ptSnake[0].y = 0;
    m_snakeLen = 2;

    m_snakeCurDir     = SNAKERIGHT;
    m_snakeCurDirTemp = SNAKERIGHT;
}

/**
 * clear snake food buffer
 */
static void snake_food_clear(void)
{
    rt_uint16_t i;

    for (i = 0; i < SNAKEGAME_SNAKE_MAXFOOD; i++)
    {
        m_ptFood[i].x = -1;
        m_ptFood[i].y = -1;
    }
}

/**
 * initial snake food buffer
 */
static void snake_food_init(void)
{
    rt_uint16_t i;

    for (i = 0; i < SNAKEGAME_SNAKE_MAXFOOD; i++)
    {
        snake_create_cell(&m_ptFood[i]);
    }
}

/**
 * check a point is overlapped with food buffer
 */
static rt_uint8_t snake_food_overlay_check(struct tagSnakePoint *pPoint)
{
    rt_uint8_t i;

    for (i = 0; i < SNAKEGAME_SNAKE_MAXFOOD; i++)
    {
        if ((pPoint->x == m_ptFood[i].x) && (pPoint->y == m_ptFood[i].y))
        {
            return i;
        }
    }

    return -1;
}

/**
 * snake move
 */
static rt_err_t snake_move(void)
{
    rt_err_t   ret, retval = RT_EOK;
    rt_uint8_t id;
    rt_int32_t i;
    rt_int32_t nDir = m_snakeCurDir;
    struct tagSnakePoint ptSnake;

    rt_memcpy(&ptSnake, &m_ptSnake[0], sizeof(struct tagSnakePoint));

    switch (nDir)
    {
    case SNAKELEFT:
        if (--ptSnake.x == -1)
        {
            // ......
            ptSnake.x = 0;
            if (ptSnake.y == 0)
            {
                ptSnake.y++;
                nDir = SNAKEDOWN;
            }
            else
            {
                ptSnake.y--;
                nDir = SNAKEUP;
            }
        }
        break;

    case SNAKERIGHT:
        if (++ptSnake.x >= SNAKEGAME_SCOPE_COL)
        {
            // ......
            ptSnake.x = SNAKEGAME_SCOPE_COL - 1;
            if (ptSnake.y >= SNAKEGAME_SCOPE_ROW - 1)
            {
                ptSnake.y--;
                nDir = SNAKEUP;
            }
            else
            {
                ptSnake.y++;
                nDir = SNAKEDOWN;
            }
        }
        break;

    case SNAKEUP:
        if (--ptSnake.y == -1)
        {
            // ......
            ptSnake.y = 0;
            if (ptSnake.x == 0)
            {
                ptSnake.x++;
                nDir = SNAKERIGHT;
            }
            else
            {
                ptSnake.x--;
                nDir = SNAKELEFT;
            }
        }
        break;

    case SNAKEDOWN:
        if (++ptSnake.y >= SNAKEGAME_SCOPE_ROW)
        {
            // ......
            ptSnake.y = SNAKEGAME_SCOPE_ROW - 1;
            if (ptSnake.x >= SNAKEGAME_SCOPE_COL - 1)
            {
                ptSnake.x--;
                nDir = SNAKELEFT;
            }
            else
            {
                ptSnake.x++;
                nDir = SNAKERIGHT;
            }
        }
        break;

    default:
        return RT_ERROR;
    }

    // gameover check
    ret = snake_body_overlay_check(&ptSnake);
    if (ret == RT_ERROR)
    {
        m_bsnakeGameover = 1;
        return RT_ERROR;
    }

    // food check
    id = snake_food_overlay_check(&ptSnake);
    if (id < SNAKEGAME_SNAKE_MAXFOOD)
    {
        ret = snake_create_cell(&m_ptFood[id]);
        if (ret != RT_EOK)
        {
            return RT_ERROR;
        }
        retval = RT_EBUSY;
    }

    for (i = m_snakeLen; i > 0; i--)
    {
        rt_memcpy(&m_ptSnake[i], &m_ptSnake[i - 1], sizeof(struct tagSnakePoint));
    }
    rt_memcpy(&m_ptSnake[0], &ptSnake, sizeof(struct tagSnakePoint));

    if (retval == RT_EBUSY)
    {
        m_snakeLen++;

        m_snakeTotalScore += SNAKE_LINE_SCORE;

        rt_int32_t tempLevel = m_snakeLevel;
        m_snakeLevel = m_snakeTotalScore / SNAKE_LEVEL_STEP;
        if (m_snakeLevel >= SNAKETIMERDELAY / SNAKETIMEDECSIZE)
        {
            m_snakeLevel = SNAKETIMERDELAY / SNAKETIMEDECSIZE - 1;
        }
        if (tempLevel != m_snakeLevel)
        {
            rt_uint32_t delay = SNAKETIMERDELAY - m_snakeLevel * SNAKETIMEDECSIZE;
            rt_timer_control(m_snakeTimerID, RT_TIMER_CTRL_SET_TIME, &delay);
            //rt_kprintf("m_snakeLevel = %d\n", m_snakeLevel);
        }
    }
    m_ptSnake[m_snakeLen].x = -1;
    m_ptSnake[m_snakeLen].y = -1;

    m_snakeCurDir = nDir;
    m_snakeCurDirTemp = m_snakeCurDir;

#if 0
    for (i = 0; i < m_snakeLen; i++)
    {
        rt_kprintf("11: i = %d, x = %d, y = %d\n", i, m_ptSnake[i].x, m_ptSnake[i].y);
    }
#endif

    return retval;
}

/**
 * snake upkey callback
 */
static void snake_dir_to_up(void)
{
    if (m_snakeCurDirTemp != m_snakeCurDir)
    {
        return;
    }
    if ((m_snakeCurDir == SNAKEUP) || (m_snakeCurDir == SNAKEDOWN))
    {
        return;
    }
    m_snakeCurDir = SNAKEUP;
}

/**
 * snake downkey callback
 */
static void snake_dir_to_down(void)
{
    if (m_snakeCurDirTemp != m_snakeCurDir)
    {
        return;
    }
    if ((m_snakeCurDir == SNAKEUP) || (m_snakeCurDir == SNAKEDOWN))
    {
        return;
    }
    m_snakeCurDir = SNAKEDOWN;
}

/**
 * snake leftkey callback
 */
static void snake_dir_to_left(void)
{
    if (m_snakeCurDirTemp != m_snakeCurDir)
    {
        return;
    }
    if ((m_snakeCurDir == SNAKELEFT) || (m_snakeCurDir == SNAKERIGHT))
    {
        return;
    }
    m_snakeCurDir = SNAKELEFT;
}

/**
 * snake rightkey callback
 */
static void snake_dir_to_right(void)
{
    if (m_snakeCurDirTemp != m_snakeCurDir)
    {
        return;
    }
    if ((m_snakeCurDir == SNAKELEFT) || (m_snakeCurDir == SNAKERIGHT))
    {
        return;
    }
    m_snakeCurDir = SNAKERIGHT;
}

/**
 * snake playkey callback
 */
static void snake_play_pause(void *parameter)
{
    rt_err_t ret;

    if (m_bsnakeState == SNAKSTOP)
    {
        m_bsnakeState = SNAKPLAY;
        m_snakeTotalScore = 0;
        m_snakeLevel      = 0;
        m_bsnakeGameover  = 0;
        snake_array_clear();
        snake_arraybk_clear();
        snake_bodylist_init();
        snake_food_init();
        snake_array_fill();

        ret = rt_timer_start(m_snakeTimerID);
        RT_ASSERT(ret == RT_EOK);

        rt_uint32_t delay = SNAKETIMERDELAY - m_snakeLevel * SNAKETIMEDECSIZE;
        rt_timer_control(m_snakeTimerID, RT_TIMER_CTRL_SET_TIME, &delay);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        olpc_snake_screen_timer_stop(parameter);
#endif
    }
    else if (m_bsnakeState == SNAKPAUSE)
    {
        m_bsnakeState = SNAKPLAY;
        ret = rt_timer_start(m_snakeTimerID);
        RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        olpc_snake_screen_timer_stop(parameter);
#endif
    }
    else //if (m_bsnakeState == SNAKPLAY)
    {
        m_bsnakeState = SNAKPAUSE;
        ret = rt_timer_stop(m_snakeTimerID);
        RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        olpc_snake_screen_timer_start(parameter);
#endif
    }
}

/**
 * snake exitkey callback
 */
static void snake_exit(void *parameter)
{
    m_bsnakeState     = SNAKSTOP;
    rt_timer_stop(m_snakeTimerID);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
    olpc_snake_screen_timer_start(parameter);
#endif
}

/**
 * snake timer isr
 */
static void SnakeOnTimer(void *parameter)
{
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    olpc_data->gamecmd |= CMD_GAME_SNAKE_MOVE;
    rt_event_send(olpc_data->event, EVENT_GAME_PROCESS);
}

/**
 * snake game init
 */
static rt_err_t snake_game_init(void *parameter)
{
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    m_snakeTotalScore = 0;
    m_bsnakeState     = SNAKSTOP;
    m_snakeLevel      = 0;
    m_bsnakeGameover  = 0;

    snake_array_clear();
    snake_arraybk_clear();

    snake_bodylist_clear();
    snake_food_clear();
    snake_array_fill();

    m_snakeTimerID = rt_timer_create("game_timer",
                                     SnakeOnTimer,
                                     (void *)olpc_data,
                                     SNAKETIMERDELAY - m_snakeLevel * SNAKETIMEDECSIZE,
                                     RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(m_snakeTimerID != RT_NULL);

    return RT_EOK;
}

/**
 * snake game deinit
 */
static void snake_game_deinit(void *parameter)
{
    rt_err_t ret;

    ret = rt_timer_delete(m_snakeTimerID);
    RT_ASSERT(ret == RT_EOK);

    m_snakeTimerID = RT_NULL;
}

/**
 * olpc snake refresh process
 */
static rt_err_t olpc_game_task_fun(struct olpc_snake_data *olpc_data)
{
    rt_err_t ret;

    if ((olpc_data->gamecmd & CMD_GAME_SNAKE_MOVE) == CMD_GAME_SNAKE_MOVE)
    {
        olpc_data->gamecmd &= ~CMD_GAME_SNAKE_MOVE;

        do
        {
            ret = snake_move();

            if (ret == RT_EBUSY)
            {
                olpc_data->cmd |= CMD_UPDATE_CTRL0 | CMD_UPDATE_CTRL0_SCORE;
            }
        }
        while (ret == RT_EBUSY);

        if (ret == RT_EOK)
        {
            snake_array_clear();
            snake_array_fill();
            olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_SNAKE;
        }
        else
        {
            snake_exit(olpc_data);
            olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_OVER;
            olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_PLAY;
        }
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
    }

    if (olpc_data->gamecmd != 0)
    {
        rt_event_send(olpc_data->event, EVENT_GAME_PROCESS);
    }

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * 1bpp snake control
 *
 **************************************************************************************************
 */

/**
 * olpc note lut set.
 */
static rt_err_t olpc_snake_lutset(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut0, lut1;

    lut0.winId  = SNAKE_GRAY0_WIN;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    lut0.lut    = bpp0_lut;
    lut0.size   = sizeof(bpp0_lut) / sizeof(bpp0_lut[0]);

    lut1.winId  = SNAKE_GRAY1_WIN;
    lut1.format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    lut1.lut    = bpp1_lut;
    lut1.size   = sizeof(bpp1_lut) / sizeof(bpp1_lut[0]);

    ret = rt_display_lutset(&lut0, &lut1, RT_NULL);
    RT_ASSERT(ret == RT_EOK);

    // clear screen
    {
        struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;
        rt_device_t device = olpc_data->disp->device;
        struct rt_device_graphic_info info;

        ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
        RT_ASSERT(ret == RT_EOK);

        rt_display_win_clear(SNAKE_GRAY0_WIN, RTGRAPHIC_PIXEL_FORMAT_GRAY1, 0, WIN_LAYERS_H, 0);
    }

    return ret;
}

/**
 * olpc snake demo init.
 */
static rt_err_t olpc_snake_init(struct olpc_snake_data *olpc_data)
{
    rt_err_t    ret;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    // malloc buffer for snake region
    olpc_data->snake_fblen = SNAKE_FB_W * SNAKE_FB_H / 8;
    olpc_data->snake_fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->snake_fblen);
    RT_ASSERT(olpc_data->snake_fb != RT_NULL);
    rt_memset((void *)olpc_data->snake_fb, 0x00, olpc_data->snake_fblen);

    // malloc buffer for control region
    olpc_data->ctrl0_fblen = CTRL1_FB_W * CTRL1_FB_H / 8;
    olpc_data->ctrl0_fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->ctrl0_fblen);
    RT_ASSERT(olpc_data->ctrl0_fb != RT_NULL);
    rt_memset((void *)olpc_data->ctrl0_fb, 0x00, olpc_data->ctrl0_fblen);

    olpc_data->ctrl1_fblen = CTRL1_FB_W * CTRL1_FB_H / 8;
    olpc_data->ctrl1_fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->ctrl1_fblen);
    RT_ASSERT(olpc_data->ctrl1_fb != RT_NULL);
    rt_memset((void *)olpc_data->ctrl1_fb, 0x00, olpc_data->ctrl1_fblen);

    snake_game_init(olpc_data);

    olpc_data->cmd = CMD_UPDATE_GAME | CMD_UPDATE_GAME_BK | CMD_UPDATE_GAME_SNAKE |
                     CMD_UPDATE_CTRL0 | CMD_UPDATE_CTRL0_EXIT | CMD_UPDATE_CTRL0_SCORE |
                     CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_PLAY | CMD_UPDATE_CTRL1_UP | CMD_UPDATE_CTRL1_DOWN | CMD_UPDATE_CTRL1_LEFT | CMD_UPDATE_CTRL1_RIGHT;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

    return RT_EOK;
}

/**
 * olpc snake demo deinit.
 */
static void olpc_snake_deinit(struct olpc_snake_data *olpc_data)
{
    snake_game_deinit(olpc_data);

    rt_free_large((void *)olpc_data->ctrl1_fb);
    olpc_data->ctrl1_fb = RT_NULL;

    rt_free_large((void *)olpc_data->ctrl0_fb);
    olpc_data->ctrl0_fb = RT_NULL;

    rt_free_large((void *)olpc_data->snake_fb);
    olpc_data->snake_fb = RT_NULL;
}

/**
 * olpc snake refresh process
 */
static rt_err_t olpc_snake_game_region_refresh(struct olpc_snake_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = SNAKE_GRAY1_WIN;
    wincfg->fb    = olpc_data->snake_fb;
    wincfg->w     = ((SNAKE_FB_W + 31) / 32) * 32;
    wincfg->h     = SNAKE_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = SNAKE_REGION_X + (SNAKE_REGION_W - SNAKE_FB_W) / 2;
    wincfg->y     = SNAKE_REGION_Y + (SNAKE_REGION_H - SNAKE_FB_H) / 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 32) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->snake_fblen);

    // draw game region
    img_info = &snake_bkg_info;
    xoffset = (SNAKE_FB_W - img_info->w) / 2;
    yoffset = (SNAKE_FB_H - img_info->h) / 2;

    // draw back ground
    if ((olpc_data->cmd & CMD_UPDATE_GAME_BK) == CMD_UPDATE_GAME_BK)
    {
        olpc_data->cmd &= ~CMD_UPDATE_GAME_BK;

        RT_ASSERT(img_info->w <= wincfg->w);
        RT_ASSERT(img_info->h <= wincfg->h);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // draw snake
    xoffset = 30;
    yoffset = 30;
    if ((olpc_data->cmd & CMD_UPDATE_GAME_SNAKE) == CMD_UPDATE_GAME_SNAKE)
    {
        olpc_data->cmd &= ~CMD_UPDATE_GAME_SNAKE;

        rt_uint16_t i, j;
        for (j = 0; j < SNAKEGAME_SCOPE_ROW; j++)
        {
            for (i = 0; i < SNAKEGAME_SCOPE_COL; i++)
            {
                if (m_snakeArraybk[j][i] != m_snakeArray[j][i])
                {
                    m_snakeArraybk[j][i] = m_snakeArray[j][i];

                    if (SNAKEGAME_CELLTYPE_EMPTY == m_snakeArray[j][i])
                    {
                        img_info = &snake_blkempty_info;
                    }
                    else if (SNAKEGAME_CELLTYPE_HALF == m_snakeArray[j][i])
                    {
                        img_info = &snake_blkhalf_info;
                    }
                    else
                    {
                        img_info = &snake_blkfull_info;
                    }

                    rt_display_img_fill(img_info, wincfg->fb, wincfg->w,
                                        xoffset + img_info->x + i * img_info->w,
                                        yoffset + img_info->y + j * img_info->h);
                }
            }
        }
    }

    // draw game over
    if ((olpc_data->cmd & CMD_UPDATE_GAME_OVER) == CMD_UPDATE_GAME_OVER)
    {
        olpc_data->cmd &= ~CMD_UPDATE_GAME_OVER;

        img_info = &snake_gameover_info;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    return RT_EOK;
}

/**
 * olpc snake refresh process
 */
static rt_err_t olpc_snake_ctrl0_region_refresh(struct olpc_snake_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = SNAKE_GRAY1_WIN;
    wincfg->fb    = olpc_data->ctrl0_fb;
    wincfg->w     = ((CTRL0_FB_W + 31) / 32) * 32;
    wincfg->h     = CTRL0_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = CTRL0_REGION_X + (CTRL0_REGION_W - CTRL0_FB_W) / 2;
    wincfg->y     = CTRL0_REGION_Y + (CTRL0_REGION_H - CTRL0_FB_H) / 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 32) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->ctrl0_fblen);

    // draw score
    xoffset = 0;
    yoffset = 0;
    if ((olpc_data->cmd & CMD_UPDATE_CTRL0_SCORE) == CMD_UPDATE_CTRL0_SCORE)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL0_SCORE;

        img_info = snake_nums[(m_snakeTotalScore / 10000) % 10];
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 0), yoffset + img_info->y);

        img_info = snake_nums[(m_snakeTotalScore / 1000) % 10];
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 1), yoffset + img_info->y);

        img_info = snake_nums[(m_snakeTotalScore / 100) % 10];
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 2), yoffset + img_info->y);

        img_info = snake_nums[(m_snakeTotalScore / 10) % 10];
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 3), yoffset + img_info->y);

        img_info = snake_nums[(m_snakeTotalScore / 1) % 10];
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 4), yoffset + img_info->y);
    }

    // exit btn
    xoffset = 0;
    yoffset = 0;
    if ((olpc_data->cmd & CMD_UPDATE_CTRL0_EXIT) == CMD_UPDATE_CTRL0_EXIT)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL0_EXIT;

        img_info = &snake_exit0_info;
        if (olpc_data->btnexitsta == 1)
        {
            img_info = &snake_exit1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    return RT_EOK;
}

/**
 * olpc snake refresh process
 */
static rt_err_t olpc_snake_ctrl1_region_refresh(struct olpc_snake_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = SNAKE_GRAY1_WIN;
    wincfg->fb    = olpc_data->ctrl1_fb;
    wincfg->w     = ((CTRL1_FB_W + 31) / 32) * 32;
    wincfg->h     = CTRL1_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = CTRL1_REGION_X + (CTRL1_REGION_W - CTRL1_FB_W) / 2;
    wincfg->y     = CTRL1_REGION_Y + (CTRL1_REGION_H - CTRL1_FB_H) / 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 32) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->ctrl1_fblen);

    xoffset = 0;
    yoffset = 0;

    // up btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL1_UP) == CMD_UPDATE_CTRL1_UP)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL1_UP;

        img_info = &snake_up0_info;
        if (olpc_data->btnupsta == 1)
        {
            img_info = &snake_up1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // down btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL1_DOWN) == CMD_UPDATE_CTRL1_DOWN)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL1_DOWN;

        img_info = &snake_down0_info;
        if (olpc_data->btndownsta == 1)
        {
            img_info = &snake_down1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // left btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL1_LEFT) == CMD_UPDATE_CTRL1_LEFT)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL1_LEFT;

        img_info = &snake_left0_info;
        if (olpc_data->btnleftsta == 1)
        {
            img_info = &snake_left1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // right btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL1_RIGHT) == CMD_UPDATE_CTRL1_RIGHT)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL1_RIGHT;

        img_info = &snake_right0_info;
        if (olpc_data->btnrightsta == 1)
        {
            img_info = &snake_right1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // start btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL1_PLAY) == CMD_UPDATE_CTRL1_PLAY)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL1_PLAY;

        img_info = &snake_play0_info;
        if (m_bsnakeState == SNAKPLAY)
        {
            img_info = &snake_pause0_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    return RT_EOK;
}

/**
 * olpc snake refresh process
 */
static rt_err_t olpc_snake_task_fun(struct olpc_snake_data *olpc_data)
{
    rt_err_t ret;
    struct rt_display_config  wincfg;
    struct rt_display_config *winhead = RT_NULL;

    rt_memset(&wincfg, 0, sizeof(struct rt_display_config));

    if ((olpc_data->cmd & CMD_UPDATE_GAME) == CMD_UPDATE_GAME)
    {
        olpc_data->cmd &= ~CMD_UPDATE_GAME;
        olpc_snake_game_region_refresh(olpc_data, &wincfg);
    }
    else if ((olpc_data->cmd & CMD_UPDATE_CTRL0) == CMD_UPDATE_CTRL0)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL0;
        olpc_snake_ctrl0_region_refresh(olpc_data, &wincfg);
    }
    else if ((olpc_data->cmd & CMD_UPDATE_CTRL1) == CMD_UPDATE_CTRL1)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL1;
        olpc_snake_ctrl1_region_refresh(olpc_data, &wincfg);
    }

    //refresh screen
    rt_display_win_layers_list(&winhead, &wincfg);
    ret = rt_display_win_layers_set(winhead);
    RT_ASSERT(ret == RT_EOK);

    if (olpc_data->cmd != 0)
    {
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
    }

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * olpc snake touch functions
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)

/**
 * left button touch.
 */
static rt_err_t olpc_snake_btnleft_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnleftsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_LEFT;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        snake_dir_to_left();
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnleftsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_LEFT;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;
    default:
        break;
    }

    return ret;
}

/**
 * right button touch.
 */
static rt_err_t olpc_snake_btnright_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnrightsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_RIGHT;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        snake_dir_to_right();
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnrightsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_RIGHT;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;

    default:
        break;
    }

    return ret;
}

/**
 * up button touch.
 */
static rt_err_t olpc_snake_btnup_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnupsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_UP;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        snake_dir_to_up();
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnupsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_UP;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;

    default:
        break;
    }

    return ret;
}

/**
 * down button touch.
 */
static rt_err_t olpc_snake_btndown_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btndownsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_DOWN;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        snake_dir_to_down();
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btndownsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_DOWN;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;

    default:
        break;
    }

    return ret;
}

/**
 * start button touch.
 */
static rt_err_t olpc_snake_btnplay_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        snake_play_pause(olpc_data);

        olpc_data->cmd |= CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_PLAY;
        olpc_data->cmd |= CMD_UPDATE_CTRL0 | CMD_UPDATE_CTRL0_SCORE;
        olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_SNAKE;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;

    default:
        break;
    }

    return ret;
}

/**
 * exit button touch.
 */
static rt_err_t olpc_snake_btnexit_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnexitsta = 1;
        if (m_bsnakeState != SNAKSTOP)
        {
            snake_exit(olpc_data);

            m_snakeTotalScore = 0;
            m_snakeLevel      = 0;
            snake_array_clear();
            snake_arraybk_clear();
            snake_bodylist_clear();
            snake_food_clear();
            snake_array_fill();

            olpc_data->cmd = CMD_UPDATE_GAME | CMD_UPDATE_GAME_SNAKE |
                             CMD_UPDATE_CTRL0 | CMD_UPDATE_CTRL0_EXIT | CMD_UPDATE_CTRL0_SCORE |
                             CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_PLAY;
            rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        }
        else
        {
            olpc_data->cmd |= CMD_UPDATE_CTRL0 | CMD_UPDATE_CTRL0_EXIT;
            rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        }
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnexitsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL0 | CMD_UPDATE_CTRL0_EXIT;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        rt_thread_delay(10);
        rt_event_send(olpc_data->event, EVENT_GAME_EXIT);
        break;

    default:
        break;
    }

    return ret;
}

/**
 * touch buttons register.
 */
static rt_err_t olpc_snake_touch_register(void *parameter)
{
    image_info_t *img_info;
    rt_uint16_t   x, y;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    x = CTRL0_REGION_X + (CTRL0_REGION_W - CTRL0_FB_W) / 2;
    y = CTRL0_REGION_Y + (CTRL0_REGION_H - CTRL0_FB_H) / 2;

    /* exit button touch register */
    img_info = &snake_exit0_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_snake_btnexit_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);


    x = CTRL1_REGION_X + (CTRL1_REGION_W - CTRL1_FB_W) / 2;
    y = CTRL1_REGION_Y + (CTRL1_REGION_H - CTRL1_FB_H) / 2;

    /* left button touch register */
    img_info = &snake_left0_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_snake_btnleft_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* right button touch register */
    img_info = &snake_right0_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_snake_btnright_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* up button touch register */
    img_info = &snake_up0_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_snake_btnup_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* down button touch register */
    img_info = &snake_down0_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_snake_btndown_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* start button touch register */
    img_info = &snake_play0_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_snake_btnplay_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    return RT_EOK;
}

/**
 * touch buttons unregister.
 */
static rt_err_t olpc_snake_touch_unregister(void *parameter)
{
    image_info_t *img_info;

    img_info = &snake_exit0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &snake_up0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &snake_down0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &snake_left0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &snake_right0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &snake_play0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

#if defined(OLPC_APP_SRCSAVER_ENABLE)
static image_info_t screen_item;
static rt_err_t olpc_snake_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    switch (event)
    {
    default:
        break;
    }

#if defined(OLPC_APP_SRCSAVER_ENABLE)
    if ((olpc_data) && (olpc_data->srctimer))
    {
        rt_timer_start(olpc_data->srctimer);    //reset screen protection timer
    }
#endif

    return ret;
}

static rt_err_t olpc_snake_screen_touch_register(void *parameter)
{
    image_info_t *img_info = &screen_item;
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button touch register */
    {
        screen_item.w = WIN_LAYERS_W;
        screen_item.h = WIN_LAYERS_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_snake_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_snake_screen_touch_unregister(void *parameter)
{
    image_info_t *img_info = &screen_item;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}
#endif
#endif

/*
 **************************************************************************************************
 *
 * olpc snake screen protection functions
 *
 **************************************************************************************************
 */
#if defined(OLPC_APP_SRCSAVER_ENABLE)

/**
 * snake screen protection timeout callback.
 */
static void olpc_snake_srcprotect_timerISR(void *parameter)
{
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;
    rt_event_send(olpc_data->event, EVENT_SRCSAVER_ENTER);
}

/**
 * exit screen protection hook.
 */
static rt_err_t olpc_snake_screen_protection_hook(void *parameter)
{
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    rt_event_send(olpc_data->event, EVENT_SRCSAVER_EXIT);

    return RT_EOK;
}

static void olpc_snake_screen_timer_start(void *parameter)
{
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

#if defined(RT_USING_PISCES_TOUCH)
    olpc_snake_screen_touch_register(parameter);
#endif

    rt_timer_start(olpc_data->srctimer);
}

static void olpc_snake_screen_timer_stop(void *parameter)
{
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    rt_timer_stop(olpc_data->srctimer);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_snake_screen_touch_unregister(parameter);
#endif
}

/**
 * start screen protection.
 */
static rt_err_t olpc_snake_screen_protection_enter(void *parameter)
{
    olpc_snake_screen_timer_stop(parameter);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_snake_touch_unregister(parameter);
    olpc_touch_list_clear();
#endif

    olpc_srcprotect_app_init(olpc_snake_screen_protection_hook, parameter);

    return RT_EOK;
}

/**
 * exit screen protection.
 */
static rt_err_t olpc_snake_screen_protection_exit(void *parameter)
{
    struct olpc_snake_data *olpc_data = (struct olpc_snake_data *)parameter;

    olpc_snake_lutset(olpc_data);    //reset lut

#if defined(RT_USING_PISCES_TOUCH)
    olpc_snake_touch_register(parameter);
#endif
    olpc_snake_screen_timer_start(parameter);

    snake_arraybk_clear();

    olpc_data->cmd = CMD_UPDATE_GAME | CMD_UPDATE_GAME_BK | CMD_UPDATE_GAME_SNAKE |
                     CMD_UPDATE_CTRL0 | CMD_UPDATE_CTRL0_EXIT | CMD_UPDATE_CTRL0_SCORE |
                     CMD_UPDATE_CTRL1 | CMD_UPDATE_CTRL1_PLAY | CMD_UPDATE_CTRL1_UP | CMD_UPDATE_CTRL1_DOWN | CMD_UPDATE_CTRL1_LEFT | CMD_UPDATE_CTRL1_RIGHT;
    if (m_bsnakeGameover)
    {
        olpc_data->cmd |= CMD_UPDATE_GAME_OVER;
    }
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

    return RT_EOK;
}
#endif

/*
 **************************************************************************************************
 *
 * olpc snake demo init & thread
 *
 **************************************************************************************************
 */

/**
 * olpc snake dmeo thread.
 */
static void olpc_snake_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_snake_data *olpc_data;

    olpc_data = (struct olpc_snake_data *)rt_malloc(sizeof(struct olpc_snake_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_snake_data));

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_snake_lutset(olpc_data);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->event = rt_event_create("snake_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->event != RT_NULL);

    ret = olpc_snake_init(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_snake_touch_register(olpc_data);
#endif

#if defined(OLPC_APP_SRCSAVER_ENABLE)
    olpc_data->srctimer = rt_timer_create("snakeprotect",
                                          olpc_snake_srcprotect_timerISR,
                                          (void *)olpc_data,
                                          SNAKE_SRCSAVER_TIME,
                                          RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->srctimer != RT_NULL);
    olpc_snake_screen_timer_start(olpc_data);
#endif

    while (1)
    {
        ret = rt_event_recv(olpc_data->event,
                            EVENT_GAME_PROCESS | EVENT_GAME_EXIT | EVENT_DISPLAY_REFRESH | EVENT_SRCSAVER_ENTER | EVENT_SRCSAVER_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_GAME_PROCESS)
        {
            ret = olpc_game_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_DISPLAY_REFRESH)
        {
            ret = olpc_snake_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        if (event & EVENT_SRCSAVER_ENTER)
        {
            ret = olpc_snake_screen_protection_enter(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_SRCSAVER_EXIT)
        {
            ret = olpc_snake_screen_protection_exit(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }
#endif
        if (event & EVENT_GAME_EXIT)
        {
            break;
        }
    }

    /* Thread deinit */
#if defined(OLPC_APP_SRCSAVER_ENABLE)
    rt_timer_stop(olpc_data->srctimer);
    ret = rt_timer_delete(olpc_data->srctimer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->srctimer = RT_NULL;
#endif

#if defined(RT_USING_PISCES_TOUCH)
    olpc_snake_touch_unregister(olpc_data);
    olpc_touch_list_clear();
#endif

    olpc_snake_deinit(olpc_data);

    rt_event_delete(olpc_data->event);
    olpc_data->event = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;

    rt_event_send(olpc_main_event, EVENT_APP_CLOCK);
}

/**
 * olpc snake demo application init.
 */
#if defined(OLPC_DLMODULE_ENABLE)
SECTION(".param") rt_uint16_t dlmodule_thread_priority = 5;
SECTION(".param") rt_uint32_t dlmodule_thread_stacksize = 2048;
int main(int argc, char *argv[])
{
    olpc_snake_thread(RT_NULL);
    return RT_EOK;
}

#else
int olpc_snake_app_init(void)
{
    rt_thread_t rtt_snake;

    rtt_snake = rt_thread_create("snake", olpc_snake_thread, RT_NULL, 2048 * 2, 5, 10);
    RT_ASSERT(rtt_snake != RT_NULL);
    rt_thread_startup(rtt_snake);

    return RT_EOK;
}
//INIT_APP_EXPORT(olpc_snake_app_init);
#endif

#endif
