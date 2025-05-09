/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)

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
static uint32_t bpp1_lut[2] =
{
    0x000000, 0x00FFFFFF
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
 * Game region:(23,40) (756,1113), block 52x51
 * Next region:(797,432) (963, 599), block 22x22
 *
 **************************************************************************************************
 */
#define BLOCK_GRAY1_WIN     0

#define BLOCK_REGION_X      0
#define BLOCK_REGION_Y      60
#define BLOCK_REGION_W      WIN_LAYERS_W
#define BLOCK_REGION_H      1154
#define BLOCK_FB_W          ((WIN_LAYERS_W / 32) * 32)    /* block frame buffer w */
#define BLOCK_FB_H          BLOCK_REGION_H                /* block frame buffer h */

#define CTRL_REGION_X       0
#define CTRL_REGION_Y       1400
#define CTRL_REGION_W       WIN_LAYERS_W
#define CTRL_REGION_H       360
#define CTRL_FB_W           ((WIN_LAYERS_W / 32) * 32)    /* control frame buffer w */
#define CTRL_FB_H           CTRL_REGION_H                 /* control frame buffer h */

/* Event define */
#define EVENT_DISPLAY_REFRESH   (0x01UL << 0)
#define EVENT_GAME_PROCESS      (0x01UL << 1)
#define EVENT_GAME_EXIT         (0x01UL << 2)
#define EVENT_SRCSAVER_ENTER    (0x01UL << 3)
#define EVENT_SRCSAVER_EXIT     (0x01UL << 4)

/* Game command define */
#define CMD_GAME_MOVE_DOWN      (0x01UL << 0)

/* Display command define */
#define CMD_UPDATE_CTRL         (0x01UL << 0)
#define CMD_UPDATE_GAME         (0x01UL << 1)

#define CMD_UPDATE_GAME_BK      (0x01UL << 2)
#define CMD_UPDATE_GAME_SCORE   (0x01UL << 3)
#define CMD_UPDATE_GAME_HSCORE  (0x01UL << 4)
#define CMD_UPDATE_GAME_LEVEL   (0x01UL << 5)
#define CMD_UPDATE_GAME_SPEED   (0x01UL << 6)
#define CMD_UPDATE_GAME_NEXT    (0x01UL << 7)
#define CMD_UPDATE_GAME_BLOCK   (0x01UL << 8)
#define CMD_UPDATE_GAME_OVER    (0x01UL << 9)

#define CMD_UPDATE_CTRL_BK      (0x01UL << 10)
#define CMD_UPDATE_CTRL_START   (0x01UL << 11)
#define CMD_UPDATE_CTRL_EXIT    (0x01UL << 12)
#define CMD_UPDATE_CTRL_PAUSE   (0x01UL << 13)
#define CMD_UPDATE_CTRL_UP      (0x01UL << 14)
#define CMD_UPDATE_CTRL_DOWN    (0x01UL << 15)
#define CMD_UPDATE_CTRL_LEFT    (0x01UL << 16)
#define CMD_UPDATE_CTRL_RIGHT   (0x01UL << 17)

#define BLOCK_SRCSAVER_TIME     (RT_TICK_PER_SECOND * 15)
/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern image_info_t block_bkg_info;
extern image_info_t block_big0_info;
extern image_info_t block_big1_info;
extern image_info_t block_small0_info;
extern image_info_t block_small1_info;
extern image_info_t block_gameover_info;

extern image_info_t block_num0_info;
extern image_info_t block_num1_info;
extern image_info_t block_num2_info;
extern image_info_t block_num3_info;
extern image_info_t block_num4_info;
extern image_info_t block_num5_info;
extern image_info_t block_num6_info;
extern image_info_t block_num7_info;
extern image_info_t block_num8_info;
extern image_info_t block_num9_info;

extern image_info_t block_ctrl_info;
extern image_info_t block_btnup0_info;
extern image_info_t block_btnup1_info;
extern image_info_t block_btndown0_info;
extern image_info_t block_btndown1_info;
extern image_info_t block_btnleft0_info;
extern image_info_t block_btnleft1_info;
extern image_info_t block_btnright0_info;
extern image_info_t block_btnright1_info;
extern image_info_t block_start0_info;
extern image_info_t block_start1_info;
extern image_info_t block_exit0_info;
extern image_info_t block_exit1_info;
extern image_info_t block_pause0_info;
extern image_info_t block_pause1_info;

static void block_game_stop(void *parameter);
static void block_game_draw_block(void *parameter);
static void block_game_show_level(void *parameter);
static void block_game_show_score(void *parameter);
static void block_game_show_hiscore(void *parameter);
static void block_game_draw_next(void *parameter);
static void block_game_show_speed(void *parameter);
static void block_game_show_over(void *parameter);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
static void olpc_block_screen_timer_start(void *parameter);
static void olpc_block_screen_timer_stop(void *parameter);
#endif

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_block_data
{
    rt_display_data_t disp;
    rt_timer_t    srctimer;

    rt_event_t  event;
    rt_uint32_t cmd;
    rt_uint32_t gamecmd;

    rt_uint8_t *block_fb;
    rt_uint32_t block_fblen;

    rt_uint8_t *ctrl_fb;
    rt_uint32_t ctrl_fblen;

    rt_uint8_t  btnupsta;
    rt_uint8_t  btndownsta;
    rt_uint8_t  btnleftsta;
    rt_uint8_t  btnrightsta;
    rt_uint8_t  btnstartsta;
    rt_uint8_t  btnexitsta;
    rt_uint8_t  btnpausesta;
};

static image_info_t *block_nums[10] =
{
    &block_num0_info,
    &block_num1_info,
    &block_num2_info,
    &block_num3_info,
    &block_num4_info,
    &block_num5_info,
    &block_num6_info,
    &block_num7_info,
    &block_num8_info,
    &block_num9_info,
};

/*
 **************************************************************************************************
 *
 * block game
 *
 **************************************************************************************************
 */
#define BLOCKGAME_SCOPE_COL     14
#define BLOCKGAME_SCOPE_ROW     21

#define BLOCKGAME_STOP          (1<<0)
#define BLOCKGAME_RUN           (1<<1)
#define BLOCKGAME_PAUSE         (1<<2)

#define BLOCK_IMAGE_NULL        0xFFFF
#define BLOCK_LIMIT_MASK        0x8001
#define BLOCK_BIT_MASK          0x4000

#define BLOCKTIMERDELAY         1000
#define BLOCKTIMEDECSIZE        100

#define BLOCK_LINE_SCORE        140
#define BLOCK_LEVEL_STEP        (BLOCK_LINE_SCORE * 100)

#define BLOCK_INIT_POS_X        6

#define BLOCKRANDFUNC()         rand()

unsigned int  BlockNextMat[4];
unsigned int  BlockScopeBuf[BLOCKGAME_SCOPE_ROW][BLOCKGAME_SCOPE_COL];
unsigned int  BlockScopeBufBk[BLOCKGAME_SCOPE_ROW][BLOCKGAME_SCOPE_COL];
unsigned int  BlockInsertFlag[BLOCKGAME_SCOPE_ROW + 4];
unsigned int  CurBlock;
unsigned int  BlockAspect;
unsigned char BlockGameLevel;
unsigned char BlockGameLevelBase;
unsigned long BlockGameScore;
unsigned long BlockGameHiScore = 0;;
unsigned char BlockGameState;
unsigned int  MoveBlock_X;
unsigned int  MoveBlock_Y;
unsigned int  BlockFlashing;
unsigned int  BlockGameover = 0;

rt_timer_t    m_blockTimerID = RT_NULL;

/**
 * block shape
 */
static unsigned int BlockDat[7][4][4] =
{
    {
        {0x04, 0x07, 0x00, 0x00},   // 0 X 0 0
        {0x06, 0x04, 0x04, 0x00},   // 0 X X X
        {0x0E, 0x02, 0x00, 0x00},   // 0 0 0 0
        {0x02, 0x02, 0x06, 0x00},   // 0 0 0 0
    },
    {
        {0x02, 0x0E, 0x00, 0x00},   // 0 0 X 0
        {0x04, 0x04, 0x06, 0x00},   // X X X 0
        {0x07, 0x04, 0x00, 0x00},   // 0 0 0 0
        {0x06, 0x02, 0x02, 0x00},   // 0 0 0 0
    },
    {
        {0x06, 0x06, 0x00, 0x00},   // 0 X X 0
        {0x06, 0x06, 0x00, 0x00},   // 0 X X 0
        {0x06, 0x06, 0x00, 0x00},   // 0 0 0 0
        {0x06, 0x06, 0x00, 0x00},   // 0 0 0 0
    },
    {
        {0x06, 0x0C, 0x00, 0x00},   // 0 X X 0
        {0x04, 0x06, 0x02, 0x00},   // X X 0 0
        {0x06, 0x0C, 0x00, 0x00},   // 0 0 0 0
        {0x04, 0x06, 0x02, 0x00},   // 0 0 0 0
    },
    {
        {0x06, 0x03, 0x00, 0x00},   // 0 X X 0
        {0x02, 0x06, 0x04, 0x00},   // 0 0 X X
        {0x06, 0x03, 0x00, 0x00},   // 0 0 0 0
        {0x02, 0x06, 0x04, 0x00},   // 0 0 0 0
    },
    {
        {0x02, 0x07, 0x00, 0x00},   // 0 0 X 0
        {0x04, 0x06, 0x04, 0x00},   // 0 X X X
        {0x07, 0x02, 0x00, 0x00},   // 0 0 0 0
        {0x02, 0x06, 0x02, 0x00},   // 0 0 0 0
    },
    {
        {0x00, 0x0F, 0x00, 0x00},   // 0 0 0 0
        {0x01, 0x01, 0x01, 0x01},   // X X X X
        {0x00, 0x0F, 0x00, 0x00},   // 0 0 0 0
        {0x01, 0x01, 0x01, 0x01},   // 0 0 0 0
    },
};

static void BlockInitNextMat(void)
{
    int i, temp;

    for (i = 0; i < 4; i++)
    {
        temp  = (BLOCKRANDFUNC() % 7) & 0xff;
        temp |= (((BLOCKRANDFUNC() % 4) << 8) & 0xff00);
        BlockNextMat[i] = temp;
    }
}

static void BlockRefreshNextMat(unsigned int justrefresh)
{
    int i = 0;

    if (justrefresh == 0)
    {
        CurBlock    = BlockNextMat[0] & 0xff;
        BlockAspect = (BlockNextMat[0] >> 8) & 0xff;
        MoveBlock_Y = 0;
        MoveBlock_X = BLOCK_INIT_POS_X;
    }

    for (i = 0; i < 3; i++)
    {
        BlockNextMat[i] = BlockNextMat[i + 1];
    }

    BlockNextMat[3] = ((BLOCKRANDFUNC() % 7) & 0xff) | (((BLOCKRANDFUNC() % 4) << 8) & 0xff00);
}

/**
 * olpc block demo init.
 */
static void BlockValueInit(void)
{
    unsigned int i, j;

    for (j = 0; j < BLOCKGAME_SCOPE_ROW; j++)
    {
        for (i = 0; i < BLOCKGAME_SCOPE_COL; i++)
        {
            BlockScopeBuf[j][i] = BLOCK_IMAGE_NULL;
            BlockScopeBufBk[j][i] = 0;
        }
        BlockInsertFlag[j] = 0;
    }

    BlockInsertFlag[j++] = 0xFFFF;
    BlockInsertFlag[j++] = 0xFFFF;
    BlockInsertFlag[j++] = 0xFFFF;
    BlockInsertFlag[j++] = 0xFFFF;
    CurBlock             = 0;
    BlockAspect          = 0;

    BlockInitNextMat();

    BlockGameLevel       = 0;
    BlockGameLevelBase   = 0;
    BlockGameScore       = 0;
    //BlockGameHiScore     = 0;
    BlockGameState       = BLOCKGAME_STOP;
    MoveBlock_X          = BLOCK_INIT_POS_X;
    MoveBlock_Y          = 0;
    BlockFlashing        = 0;
    BlockGameover        = 0;
}

static unsigned int BlockInsert(void)
{
    unsigned int i, j;
    unsigned int BMask;
    unsigned int ChkBit = 0;
    unsigned int BlockTemp;
    unsigned int *pBlockScopeBuf;
    unsigned int *pBlockInsertFlag;
    unsigned int *pBlock;

    pBlockInsertFlag = &BlockInsertFlag[MoveBlock_Y];
    pBlock           = &BlockDat[CurBlock][BlockAspect][0];
    for (i = 0; i < 4; i++)
    {
        BlockTemp = ((*pBlock) << MoveBlock_X);
        if (BlockTemp & BLOCK_LIMIT_MASK)
        {
            ChkBit = 1;
            break;
        }

        if ((BlockTemp + (*pBlockInsertFlag)) != (BlockTemp | (*pBlockInsertFlag)))
        {
            ChkBit = 1;
            break;
        }

        pBlockInsertFlag++;
        pBlock++;
    }

    if (ChkBit == 0)
    {
        pBlock           = &BlockDat[CurBlock][BlockAspect][0];
        pBlockScopeBuf   = &BlockScopeBuf[MoveBlock_Y][0];
        pBlockInsertFlag = &BlockInsertFlag[MoveBlock_Y];

        for (i = 0; i < 4; i++)
        {
            pBlockScopeBuf = &BlockScopeBuf[MoveBlock_Y + i][0];
            BlockTemp = ((*pBlock) << MoveBlock_X);
            *pBlockInsertFlag |= BlockTemp;
            BMask = BLOCK_BIT_MASK;
            for (j = 0; j < BLOCKGAME_SCOPE_COL; j++)
            {
                if (BlockTemp & BMask)
                {
                    *(pBlockScopeBuf + j) = CurBlock;
                }
                BMask >>= 1;
            }
            pBlockInsertFlag++;
            pBlock++;
        }
    }
    return (ChkBit);
}

static unsigned int BlockChk(unsigned int mode, unsigned int x, unsigned int y)
{
    unsigned int i, j;
    unsigned int BlockTemp;
    unsigned int BMask = BLOCK_BIT_MASK;
    unsigned int *pBlock;
    unsigned int *pBlockScopeBuf;
    unsigned int *pBlockInsertFlag;
    unsigned int ChkBit = 0;

    pBlock           = &BlockDat[CurBlock][mode][0];
    pBlockScopeBuf   = &BlockScopeBuf[y][0];
    pBlockInsertFlag = &BlockInsertFlag[y];
    for (i = 0; i < 4; i++)
    {
        pBlockScopeBuf = &BlockScopeBuf[y + i][0];
        BlockTemp = ((*pBlock) << x);
        *pBlockInsertFlag &= ~(BlockTemp | BLOCK_LIMIT_MASK);
        BMask = BLOCK_BIT_MASK;
        for (j = 0; j < BLOCKGAME_SCOPE_COL; j++)
        {
            if (BlockTemp & BMask)
            {
                *(pBlockScopeBuf + j) = BLOCK_IMAGE_NULL;
            }
            BMask >>= 1;
        }
        pBlockInsertFlag++;
        pBlock++;
    }

    pBlockInsertFlag = &BlockInsertFlag[MoveBlock_Y];
    pBlock           = &BlockDat[CurBlock][BlockAspect][0];
    for (i = 0; i < 4; i++)
    {
        BlockTemp = ((*pBlock) << MoveBlock_X);
        if (BlockTemp)
        {
            j = MoveBlock_Y + i;
        }

        if (BlockTemp & BLOCK_LIMIT_MASK)
        {
            ChkBit = 1;
            break;
        }
        if ((BlockTemp + (*pBlockInsertFlag)) != (BlockTemp | (*pBlockInsertFlag)))
        {
            ChkBit = 1;
            break;
        }

        pBlockInsertFlag++;
        pBlock++;
    }
    if (j >= BLOCKGAME_SCOPE_ROW)
    {
        ChkBit = 1;
    }

    if (ChkBit == 0)
    {
        pBlock           = &BlockDat[CurBlock][BlockAspect][0];
        pBlockScopeBuf   = &BlockScopeBuf[MoveBlock_Y][0];
        pBlockInsertFlag = &BlockInsertFlag[MoveBlock_Y];

        for (i = 0; i < 4; i++)
        {
            pBlockScopeBuf = &BlockScopeBuf[MoveBlock_Y + i][0];
            BlockTemp = ((*pBlock) << MoveBlock_X);
            *pBlockInsertFlag |= BlockTemp;
            BMask = BLOCK_BIT_MASK;
            for (j = 0; j < BLOCKGAME_SCOPE_COL; j++)
            {
                if (BlockTemp & BMask)
                {
                    *(pBlockScopeBuf + j) = CurBlock;
                }
                BMask >>= 1;
            }
            pBlockInsertFlag++;
            pBlock++;
        }
    }
    else
    {
        pBlock           = &BlockDat[CurBlock][mode][0];
        pBlockScopeBuf   = &BlockScopeBuf[y][0];
        pBlockInsertFlag = &BlockInsertFlag[y];
        for (i = 0; i < 4; i++)
        {
            pBlockScopeBuf = &BlockScopeBuf[y + i][0];
            BlockTemp = ((*pBlock) << x);
            *pBlockInsertFlag |= BlockTemp;
            BMask = BLOCK_BIT_MASK;
            for (j = 0; j < BLOCKGAME_SCOPE_COL; j++)
            {
                if (BlockTemp & BMask)
                {
                    *(pBlockScopeBuf + j) = CurBlock;
                }
                BMask >>= 1;
            }
            pBlockInsertFlag++;
            pBlock++;
        }
    }
    return (ChkBit);
}

static void BlockRefLine(unsigned int yy)
{
    unsigned int i, j;
    unsigned int *pBlockBuf;
    unsigned int *pBlockBuf2;
    unsigned int *pBlockInsertFlag;

    pBlockBuf         = &BlockScopeBuf[yy][0];
    pBlockInsertFlag  = &BlockInsertFlag[yy];
    *pBlockInsertFlag = 0;

    for (j = 0; j < BLOCKGAME_SCOPE_COL; j++)
    {
        *pBlockBuf++ = BLOCK_IMAGE_NULL;
    }

    pBlockBuf        = &BlockScopeBuf[yy][0];
    pBlockInsertFlag = &BlockInsertFlag[yy];
    for (i = yy; i > 0; i--)
    {
        pBlockBuf         = &BlockScopeBuf[i][0];
        pBlockBuf2        = &BlockScopeBuf[i - 1][0];
        *pBlockInsertFlag = *(pBlockInsertFlag - 1);
        for (j = 0; j < BLOCKGAME_SCOPE_COL; j++)
        {
            *pBlockBuf++ = *pBlockBuf2++;
        }
        pBlockInsertFlag--;
    }

    pBlockBuf         = &BlockScopeBuf[0][0];
    pBlockInsertFlag  = &BlockInsertFlag[0];
    *pBlockInsertFlag = 0;
    for (j = 0; j < BLOCKGAME_SCOPE_COL; j++)
    {
        *pBlockBuf++ = BLOCK_IMAGE_NULL;
    }
}

static unsigned int BlockRemove(void)
{
    unsigned int ret = 0;
    unsigned int i;
    unsigned int y_tmp, deldat[4];
    unsigned int RemoveLines = 0;
    unsigned int *pBlockInsertFlag;

    y_tmp            = MoveBlock_Y;
    pBlockInsertFlag = &BlockInsertFlag[y_tmp];

    BlockFlashing = 1;
    for (i = 0; i < 4; i++)
    {
        if ((*pBlockInsertFlag & ~BLOCK_LIMIT_MASK) == ((~BLOCK_LIMIT_MASK) & 0xFFFF))
        {
            deldat[RemoveLines] = y_tmp;
            RemoveLines++;
        }

        if (++y_tmp >= BLOCKGAME_SCOPE_ROW)
        {
            break;
        }
        pBlockInsertFlag++;
    }

    if (RemoveLines)
    {
        for (i = 0; i < RemoveLines; i++)
        {
            BlockRefLine(deldat[i]);
        }
    }
    BlockFlashing = 0;

    if (RemoveLines)
    {
        i = (1 << (RemoveLines - 1));
        i = i * 2;
        BlockGameScore += (unsigned long)i * BLOCK_LINE_SCORE - BLOCK_LINE_SCORE;

        if (BlockGameScore > BlockGameHiScore)
        {
            BlockGameHiScore = BlockGameScore;
        }

        BlockGameLevel = BlockGameScore / BLOCK_LEVEL_STEP + BlockGameLevelBase;
        if (BlockGameLevel >= BLOCKTIMERDELAY / BLOCKTIMEDECSIZE)
        {
            BlockGameLevel = BLOCKTIMERDELAY / BLOCKTIMEDECSIZE - 1;
        }

        ret = 1;
    }

    return ret;
}

static unsigned int BlockGetNewBlock(void)
{
    BlockRefreshNextMat(0);

    if (BlockGameState == BLOCKGAME_RUN)
    {
        if (BlockInsert())
        {
            return 1;
        }
    }
    return 0;
}

/*
 **************************************************************************************************
 *
 * 1bpp block control API
 *
 **************************************************************************************************
 */
/**
 * up key
 */
static void block_game_shape_rotate(void *parameter)
{
    unsigned int temp;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    if ((BlockGameState != BLOCKGAME_RUN) || (BlockFlashing == 1))
    {
        return;
    }

    temp = BlockAspect;
    if (++BlockAspect >= 4)
    {
        BlockAspect = 0;
    }

    if (BlockChk(temp, MoveBlock_X, MoveBlock_Y))
    {
        BlockAspect = temp;
    }

    block_game_draw_block(olpc_data);
}

/**
 * left key
 */
static void block_game_move_left(void *parameter)
{
    unsigned int temp;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    if ((BlockGameState != BLOCKGAME_RUN) || (BlockFlashing == 1))
    {
        return;
    }

    temp = MoveBlock_X;
    if (MoveBlock_X < BLOCKGAME_SCOPE_COL + 4)
    {
        MoveBlock_X++;
    }

    if (BlockChk(BlockAspect, temp, MoveBlock_Y))
    {
        MoveBlock_X = temp;
    }

    block_game_draw_block(olpc_data);
}

/**
 * right key
 */
static void block_game_move_right(void *parameter)
{
    unsigned int temp;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    if ((BlockGameState != BLOCKGAME_RUN) || (BlockFlashing == 1))
    {
        return;
    }

    temp = MoveBlock_X;
    if (MoveBlock_X > 0)
    {
        MoveBlock_X--;
    }

    if (BlockChk(BlockAspect, temp, MoveBlock_Y))
    {
        MoveBlock_X = temp;
    }

    block_game_draw_block(olpc_data);
}

/**
 * down key
 */
static void block_game_move_down(void *parameter)
{
    unsigned int temp;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    if ((BlockGameState != BLOCKGAME_RUN) || (BlockFlashing == 1))
    {
        return;
    }

    BlockGameover = 0;
    temp = MoveBlock_Y;
    MoveBlock_Y++;
    if (BlockChk(BlockAspect, MoveBlock_X, temp))
    {
        MoveBlock_Y = temp;

        unsigned int tempLevel   = BlockGameLevel;
        unsigned int tempHiScore = BlockGameHiScore;
        if (BlockRemove())
        {
            block_game_show_score(olpc_data);
            if (tempLevel != BlockGameLevel)
            {
                rt_uint32_t delay = BLOCKTIMERDELAY - BlockGameLevel * BLOCKTIMEDECSIZE;
                rt_timer_control(m_blockTimerID, RT_TIMER_CTRL_SET_TIME, &delay);

                block_game_show_level(olpc_data);
                block_game_show_speed(olpc_data);
            }
            if (tempHiScore != BlockGameHiScore)
            {
                block_game_show_hiscore(olpc_data);
            }
        }

        if (BlockGetNewBlock())
        {
            BlockGameover = 1;

            block_game_stop(olpc_data);  //game over
            block_game_show_hiscore(olpc_data);
        }

        block_game_draw_next(olpc_data);
    }

    block_game_draw_block(olpc_data);

    if (BlockGameover)
    {
        block_game_show_over(olpc_data);
    }
}

/**
 * long down key
 */
static void block_game_drop_down(void *parameter)
{
    unsigned int temp;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    if ((BlockGameState != BLOCKGAME_RUN) || (BlockFlashing == 1))
    {
        return;
    }

    BlockGameover = 0;
    do
    {
        block_game_draw_block(olpc_data);
        rt_thread_delay(5);
        temp = MoveBlock_Y;
        MoveBlock_Y++;
    }
    while (BlockChk(BlockAspect, MoveBlock_X, temp) == 0);
    MoveBlock_Y = temp;

    unsigned int tempLevel = BlockGameLevel;
    unsigned int tempHiScore = BlockGameHiScore;
    if (BlockRemove())
    {
        block_game_show_score(olpc_data);
        if (tempLevel != BlockGameLevel)
        {
            rt_uint32_t delay = BLOCKTIMERDELAY - BlockGameLevel * BLOCKTIMEDECSIZE;
            rt_timer_control(m_blockTimerID, RT_TIMER_CTRL_SET_TIME, &delay);

            block_game_show_level(olpc_data);
            block_game_show_speed(olpc_data);
        }

        if (tempHiScore != BlockGameHiScore)
        {
            block_game_show_hiscore(olpc_data);
        }
    }

    if (BlockGetNewBlock())
    {
        BlockGameover = 1;

        block_game_stop(olpc_data);  //game over
        block_game_show_hiscore(olpc_data);
    }

    block_game_draw_block(olpc_data);
    block_game_draw_next(olpc_data);

    if (BlockGameover)
    {
        block_game_show_over(olpc_data);
    }
}

/**
 * start key
 */
static void block_game_start(void *parameter)
{
    rt_err_t ret;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    if (BlockGameState == BLOCKGAME_STOP)
    {
        BlockValueInit();
        block_game_draw_block(olpc_data);

        BlockRefreshNextMat(0);
        block_game_draw_next(olpc_data);

        ret = rt_timer_start(m_blockTimerID);
        RT_ASSERT(ret == RT_EOK);
        BlockGameState = BLOCKGAME_RUN;

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        olpc_block_screen_timer_stop(parameter);
#endif
    }
}

/**
 * pause & play key
 */
static void block_game_pause(void *parameter)
{
    rt_err_t ret;

    if (BlockGameState == BLOCKGAME_PAUSE)
    {
        BlockGameState = BLOCKGAME_RUN;

        ret = rt_timer_start(m_blockTimerID);
        RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        olpc_block_screen_timer_stop(parameter);
#endif
    }
    else if (BlockGameState == BLOCKGAME_RUN)
    {
        BlockGameState = BLOCKGAME_PAUSE;

        ret = rt_timer_stop(m_blockTimerID);
        RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        olpc_block_screen_timer_start(parameter);
#endif
    }
}

/**
 * exit key
 */
static void block_game_stop(void *parameter)
{
    rt_err_t ret;

    if (BlockGameState != BLOCKGAME_STOP)
    {
        if (BlockGameState == BLOCKGAME_RUN)
        {
            ret = rt_timer_stop(m_blockTimerID);
            RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
            olpc_block_screen_timer_start(parameter);
#endif
        }
        BlockGameState = BLOCKGAME_STOP;
    }
}

/**
 * draw block area
 */
static void block_game_draw_block(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_BLOCK;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
}

/**
 * draw next shape
 */
static void block_game_draw_next(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_NEXT;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
}

/**
 * display game over
 */
static void block_game_show_over(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_OVER;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
}

/**
 * display score
 */
static void block_game_show_score(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_SCORE;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
}

/**
 * display hi-score
 */
static void block_game_show_hiscore(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_HSCORE;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
}

/**
 * display level
 */
static void block_game_show_level(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_LEVEL;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
}

/**
 * display speed
 */
static void block_game_show_speed(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_data->cmd |= CMD_UPDATE_GAME | CMD_UPDATE_GAME_SPEED;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
}

/**
 * block timer isr
 */
static void BlockOnTimer(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_data->gamecmd |= CMD_GAME_MOVE_DOWN;
    rt_event_send(olpc_data->event, EVENT_GAME_PROCESS);
}

/**
 * block game init
 */
static rt_err_t block_game_init(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    BlockValueInit();

    block_game_show_score(olpc_data);
    block_game_show_hiscore(olpc_data);
    block_game_show_level(olpc_data);
    block_game_show_speed(olpc_data);

    m_blockTimerID = rt_timer_create("game_timer",
                                     BlockOnTimer,
                                     (void *)olpc_data,
                                     BLOCKTIMERDELAY - BlockGameLevel * BLOCKTIMEDECSIZE,
                                     RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(m_blockTimerID != RT_NULL);

    return RT_EOK;
}

/**
 * block game deinit
 */
static void block_game_deinit(void *parameter)
{
    rt_err_t ret;

    ret = rt_timer_delete(m_blockTimerID);
    RT_ASSERT(ret == RT_EOK);

    m_blockTimerID = RT_NULL;
}

/**
 * olpc block refresh process
 */
static rt_err_t olpc_game_task_fun(struct olpc_block_data *olpc_data)
{
    if ((olpc_data->gamecmd & CMD_GAME_MOVE_DOWN) == CMD_GAME_MOVE_DOWN)
    {
        olpc_data->gamecmd &= ~CMD_GAME_MOVE_DOWN;
        block_game_move_down(olpc_data);
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
 * 1bpp block control
 *
 **************************************************************************************************
 */

/**
 * olpc block lut set & clear screen.
 */
static rt_err_t olpc_block_lutset(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut0;

    lut0.winId  = BLOCK_GRAY1_WIN;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    lut0.lut    = bpp1_lut;
    lut0.size   = sizeof(bpp1_lut) / sizeof(bpp1_lut[0]);

    ret = rt_display_lutset(&lut0, RT_NULL, RT_NULL);
    RT_ASSERT(ret == RT_EOK);

    // clear screen
    {
        struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;
        rt_device_t device = olpc_data->disp->device;
        struct rt_device_graphic_info info;

        ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
        RT_ASSERT(ret == RT_EOK);

        rt_display_win_clear(BLOCK_GRAY1_WIN, RTGRAPHIC_PIXEL_FORMAT_GRAY1, 0, WIN_LAYERS_H, 0);
    }

    return ret;
}

/**
 * olpc block demo init.
 */
static rt_err_t olpc_block_init(struct olpc_block_data *olpc_data)
{
    rt_err_t    ret;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    // malloc buffer for block region
    olpc_data->block_fblen = BLOCK_FB_W * BLOCK_FB_H / 8;
    olpc_data->block_fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->block_fblen);
    RT_ASSERT(olpc_data->block_fb != RT_NULL);
    rt_memset((void *)olpc_data->block_fb, 0x00, olpc_data->block_fblen);

    // malloc buffer for control region
    olpc_data->ctrl_fblen = CTRL_FB_W * CTRL_FB_H / 8;
    olpc_data->ctrl_fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->ctrl_fblen);
    RT_ASSERT(olpc_data->ctrl_fb != RT_NULL);
    rt_memset((void *)olpc_data->ctrl_fb, 0x00, olpc_data->ctrl_fblen);

    olpc_data->cmd = CMD_UPDATE_GAME | CMD_UPDATE_GAME_BK | CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_BK;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

    block_game_init(olpc_data);

    return RT_EOK;
}

/**
 * olpc block demo deinit.
 */
static void olpc_block_deinit(struct olpc_block_data *olpc_data)
{
    block_game_deinit(olpc_data);

    rt_free_large((void *)olpc_data->ctrl_fb);
    olpc_data->ctrl_fb = RT_NULL;

    rt_free_large((void *)olpc_data->block_fb);
    olpc_data->block_fb = RT_NULL;
}

/**
 * olpc block refresh process
 */
static rt_err_t olpc_block_game_region_refresh(struct olpc_block_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = BLOCK_GRAY1_WIN;
    wincfg->fb    = olpc_data->block_fb;
    wincfg->w     = ((BLOCK_FB_W + 31) / 32) * 32;
    wincfg->h     = BLOCK_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = BLOCK_REGION_X + (BLOCK_REGION_W - BLOCK_FB_W) / 2;
    wincfg->y     = BLOCK_REGION_Y + (BLOCK_REGION_H - BLOCK_FB_H) / 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 32) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->block_fblen);

    // draw game region
    img_info = &block_bkg_info;
    xoffset = (BLOCK_FB_W - img_info->w) / 2;
    yoffset = (BLOCK_FB_H - img_info->h) / 2;
    {
        // draw back ground
        if ((olpc_data->cmd & CMD_UPDATE_GAME_BK) == CMD_UPDATE_GAME_BK)
        {
            olpc_data->cmd &= ~CMD_UPDATE_GAME_BK;

            RT_ASSERT(img_info->w <= wincfg->w);
            RT_ASSERT(img_info->h <= wincfg->h);
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
        }

        // draw block
        if ((olpc_data->cmd & CMD_UPDATE_GAME_BLOCK) == CMD_UPDATE_GAME_BLOCK)
        {
            olpc_data->cmd &= ~CMD_UPDATE_GAME_BLOCK;

            rt_uint16_t i, j;
            for (j = 0; j < BLOCKGAME_SCOPE_ROW; j++)
            {
                for (i = 0; i < BLOCKGAME_SCOPE_COL; i++)
                {
                    if (BlockScopeBufBk[j][i] != BlockScopeBuf[j][i])
                    {
                        BlockScopeBufBk[j][i] = BlockScopeBuf[j][i];

                        img_info = &block_big0_info;
                        if (BlockScopeBuf[j][i] == BLOCK_IMAGE_NULL)
                        {
                            img_info = &block_big1_info;
                        }
                        rt_display_img_fill(img_info, wincfg->fb, wincfg->w,
                                            xoffset + img_info->x + (img_info->w * i),
                                            yoffset + img_info->y + (img_info->h * j));
                    }
                }
            }
        }

        // draw next
        if ((olpc_data->cmd & CMD_UPDATE_GAME_NEXT) == CMD_UPDATE_GAME_NEXT)
        {
            olpc_data->cmd &= ~CMD_UPDATE_GAME_NEXT;

            if (1) // block display next
            {
                unsigned int i, j;
                unsigned int *pBlock;
                unsigned int NBlocki, NBlockAspecti;
                unsigned int DMask = 0x08;

                NBlocki       = BlockNextMat[0] & 0xff;
                NBlockAspecti = (BlockNextMat[0] >> 8) & 0xff;
                pBlock        = &BlockDat[NBlocki][NBlockAspecti][0];
                for (j = 0; j < 4; j++)
                {
                    DMask = 0x08;
                    for (i = 0; i < 4; i++)
                    {
                        if (*pBlock & DMask)
                        {
                            img_info = &block_small0_info;
                        }
                        else
                        {
                            img_info = &block_small1_info;
                        }
                        rt_display_img_fill(img_info, wincfg->fb, wincfg->w,
                                            xoffset + img_info->x + i * img_info->w,
                                            yoffset + img_info->y + j * img_info->h);

                        DMask >>= 1;
                    }
                    pBlock++;
                }
            }
        }

        // draw score
        if ((olpc_data->cmd & CMD_UPDATE_GAME_SCORE) == CMD_UPDATE_GAME_SCORE)
        {
            olpc_data->cmd &= ~CMD_UPDATE_GAME_SCORE;

            img_info = block_nums[(BlockGameScore / 10000) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 0), yoffset + img_info->y);

            img_info = block_nums[(BlockGameScore / 1000) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 1), yoffset + img_info->y);

            img_info = block_nums[(BlockGameScore / 100) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 2), yoffset + img_info->y);

            img_info = block_nums[(BlockGameScore / 10) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 3), yoffset + img_info->y);

            img_info = block_nums[(BlockGameScore / 1) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 4), yoffset + img_info->y);
        }

        // draw hi-score
        if ((olpc_data->cmd & CMD_UPDATE_GAME_HSCORE) == CMD_UPDATE_GAME_HSCORE)
        {
            olpc_data->cmd &= ~CMD_UPDATE_GAME_HSCORE;

            rt_uint16_t yyoff = 120;

            img_info = block_nums[(BlockGameHiScore / 10000) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 0), yoffset + yyoff + img_info->y);

            img_info = block_nums[(BlockGameHiScore / 1000) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 1), yoffset + yyoff + img_info->y);

            img_info = block_nums[(BlockGameHiScore / 100) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 2), yoffset + yyoff + img_info->y);

            img_info = block_nums[(BlockGameHiScore / 10) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 3), yoffset + yyoff + img_info->y);

            img_info = block_nums[(BlockGameHiScore / 1) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x + (img_info->w * 4), yoffset + yyoff + img_info->y);
        }

        // draw level
        if ((olpc_data->cmd & CMD_UPDATE_GAME_LEVEL) == CMD_UPDATE_GAME_LEVEL)
        {
            olpc_data->cmd &= ~CMD_UPDATE_GAME_LEVEL;

            rt_uint16_t xxoff = 10;
            rt_uint16_t yyoff = 630;
            rt_uint8_t  level = BlockGameLevel + 1;

            img_info = block_nums[(level / 10) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + xxoff + img_info->x + (img_info->w * 1), yoffset + yyoff + img_info->y);

            img_info = block_nums[(level / 1) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + xxoff + img_info->x + (img_info->w * 2), yoffset + yyoff + img_info->y);
        }

        // draw speed
        if ((olpc_data->cmd & CMD_UPDATE_GAME_SPEED) == CMD_UPDATE_GAME_SPEED)
        {
            olpc_data->cmd &= ~CMD_UPDATE_GAME_SPEED;

            rt_uint16_t xxoff = 0;
            rt_uint16_t yyoff = 770;
            rt_uint16_t speed = BLOCKTIMERDELAY - BlockGameLevel * BLOCKTIMEDECSIZE;

            img_info = block_nums[(speed / 1000) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + xxoff + img_info->x + (img_info->w * 0), yoffset + yyoff + img_info->y);

            img_info = block_nums[(speed / 100) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + xxoff + img_info->x + (img_info->w * 1), yoffset + yyoff + img_info->y);

            img_info = block_nums[(speed / 10) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + xxoff + img_info->x + (img_info->w * 2), yoffset + yyoff + img_info->y);

            img_info = block_nums[(speed / 1) % 10];
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + xxoff + img_info->x + (img_info->w * 3), yoffset + yyoff + img_info->y);
        }

        // draw game over
        if ((olpc_data->cmd & CMD_UPDATE_GAME_OVER) == CMD_UPDATE_GAME_OVER)
        {
            olpc_data->cmd &= ~CMD_UPDATE_GAME_OVER;

            img_info = &block_gameover_info;
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
        }
    }
    return RT_EOK;
}

/**
 * olpc block refresh process
 */
static rt_err_t olpc_block_ctrl_region_refresh(struct olpc_block_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = BLOCK_GRAY1_WIN;
    wincfg->fb    = olpc_data->ctrl_fb;
    wincfg->w     = ((CTRL_FB_W + 31) / 32) * 32;
    wincfg->h     = CTRL_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = CTRL_REGION_X + (CTRL_REGION_W - CTRL_FB_W) / 2;
    wincfg->y     = CTRL_REGION_Y + (CTRL_REGION_H - CTRL_FB_H) / 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 32) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->ctrl_fblen);

    // back ground
    img_info = &block_ctrl_info;
    xoffset = (CTRL_FB_W - img_info->w) / 2;
    yoffset = (CTRL_FB_H - img_info->h) / 2;

    // draw back ground
    if ((olpc_data->cmd & CMD_UPDATE_CTRL_BK) == CMD_UPDATE_CTRL_BK)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL_BK;

        RT_ASSERT(img_info->w <= wincfg->w);
        RT_ASSERT(img_info->h <= wincfg->h);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }


    // up btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL_UP) == CMD_UPDATE_CTRL_UP)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL_UP;

        img_info = &block_btnup0_info;
        if (olpc_data->btnupsta == 1)
        {
            img_info = &block_btnup1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // down btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL_DOWN) == CMD_UPDATE_CTRL_DOWN)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL_DOWN;

        img_info = &block_btndown0_info;
        if (olpc_data->btndownsta == 1)
        {
            img_info = &block_btndown1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // left btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL_LEFT) == CMD_UPDATE_CTRL_LEFT)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL_LEFT;

        img_info = &block_btnleft0_info;
        if (olpc_data->btnleftsta == 1)
        {
            img_info = &block_btnleft1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // right btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL_RIGHT) == CMD_UPDATE_CTRL_RIGHT)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL_RIGHT;

        img_info = &block_btnright0_info;
        if (olpc_data->btnrightsta == 1)
        {
            img_info = &block_btnright1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // start btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL_START) == CMD_UPDATE_CTRL_START)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL_START;

        img_info = &block_start0_info;
        if (olpc_data->btnstartsta == 1)
        {
            img_info = &block_start1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // pause btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL_PAUSE) == CMD_UPDATE_CTRL_PAUSE)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL_PAUSE;

        img_info = &block_pause0_info;
        if (BlockGameState == BLOCKGAME_PAUSE)
        {
            img_info = &block_pause1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    // exit btn
    if ((olpc_data->cmd & CMD_UPDATE_CTRL_EXIT) == CMD_UPDATE_CTRL_EXIT)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL_EXIT;

        img_info = &block_exit0_info;
        if (olpc_data->btnexitsta == 1)
        {
            img_info = &block_exit1_info;
        }
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    return RT_EOK;
}

/**
 * olpc block refresh process
 */
static rt_err_t olpc_block_task_fun(struct olpc_block_data *olpc_data)
{
    rt_err_t ret;
    struct rt_display_config  wincfg;
    struct rt_display_config *winhead = RT_NULL;

    rt_memset(&wincfg, 0, sizeof(struct rt_display_config));

    if ((olpc_data->cmd & CMD_UPDATE_GAME) == CMD_UPDATE_GAME)
    {
        olpc_data->cmd &= ~CMD_UPDATE_GAME;
        olpc_block_game_region_refresh(olpc_data, &wincfg);
    }
    else if ((olpc_data->cmd & CMD_UPDATE_CTRL) == CMD_UPDATE_CTRL)
    {
        olpc_data->cmd &= ~CMD_UPDATE_CTRL;
        olpc_block_ctrl_region_refresh(olpc_data, &wincfg);
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
 * olpc block touch functions
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)

/**
 * left button touch.
 */
static rt_err_t olpc_block_btnleft_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
    case TOUCH_EVENT_LONG_DOWN:
    case TOUCH_EVENT_LONG_PRESS:
        olpc_data->btnleftsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_LEFT;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        block_game_move_left(olpc_data);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnleftsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_LEFT;
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
static rt_err_t olpc_block_btnright_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
    case TOUCH_EVENT_LONG_DOWN:
    case TOUCH_EVENT_LONG_PRESS:
        olpc_data->btnrightsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_RIGHT;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        block_game_move_right(olpc_data);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnrightsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_RIGHT;
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
static rt_err_t olpc_block_btnup_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnupsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_UP;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        block_game_shape_rotate(olpc_data);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnupsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_UP;
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
static rt_err_t olpc_block_btndown_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btndownsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_DOWN;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btndownsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_DOWN;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        block_game_drop_down(olpc_data);
        break;
    default:
        break;
    }

    return ret;
}

/**
 * start button touch.
 */
static rt_err_t olpc_block_btnstart_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnstartsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_START;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;
    case TOUCH_EVENT_UP:
        block_game_start(olpc_data);

        olpc_data->btnstartsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_START;
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
static rt_err_t olpc_block_btnexit_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnexitsta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_EXIT;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        block_game_stop(olpc_data);

        olpc_data->btnexitsta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_EXIT | CMD_UPDATE_CTRL_PAUSE;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

        // clear game
        BlockValueInit();
        block_game_draw_block(olpc_data);
        block_game_show_score(olpc_data);
        block_game_show_level(olpc_data);
        block_game_show_hiscore(olpc_data);
        block_game_draw_block(olpc_data);

        rt_thread_delay(10);
        rt_event_send(olpc_data->event, EVENT_GAME_EXIT);
        break;
    default:
        break;
    }

    return ret;
}

/**
 * pause button touch.
 */
static rt_err_t olpc_block_btnpause_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnpausesta = 1;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_PAUSE;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        block_game_pause(olpc_data);

        olpc_data->btnpausesta = 0;
        olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_PAUSE;
        rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);
        break;
    default:
        break;
    }

    return ret;
}

/**
 * touch buttons register.
 */
static rt_err_t olpc_block_touch_register(void *parameter)
{
    image_info_t *img_info;
    rt_uint16_t   x, y;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    x = CTRL_REGION_X + (CTRL_REGION_W - CTRL_FB_W) / 2;
    y = CTRL_REGION_Y + (CTRL_REGION_H - CTRL_FB_H) / 2;

    /* left button touch register */
    img_info = &block_btnleft1_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_block_btnleft_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* right button touch register */
    img_info = &block_btnright1_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_block_btnright_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* up button touch register */
    img_info = &block_btnup1_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_block_btnup_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* down button touch register */
    img_info = &block_btndown1_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_block_btndown_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* start button touch register */
    img_info = &block_start1_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_block_btnstart_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* exit button touch register */
    img_info = &block_exit1_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_block_btnexit_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);

    /* pause button touch register */
    img_info = &block_pause1_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_block_btnpause_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);
    return RT_EOK;
}

/**
 * touch buttons unregister.
 */
static rt_err_t olpc_block_touch_unregister(void *parameter)
{
    image_info_t *img_info;

    img_info = &block_btnup1_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &block_btndown1_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &block_btnleft1_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &block_btnright1_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &block_start1_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &block_exit1_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &block_pause1_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

#if defined(OLPC_APP_SRCSAVER_ENABLE)
static image_info_t screen_item;
static rt_err_t olpc_block_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

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

static rt_err_t olpc_block_screen_touch_register(void *parameter)
{
    image_info_t *img_info = &screen_item;
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button touch register */
    {
        screen_item.w = WIN_LAYERS_W;
        screen_item.h = WIN_LAYERS_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_block_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_block_screen_touch_unregister(void *parameter)
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
 * olpc block screen protection functions
 *
 **************************************************************************************************
 */
#if defined(OLPC_APP_SRCSAVER_ENABLE)

/**
 * block screen protection timeout callback.
 */
static void olpc_block_srcprotect_timerISR(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;
    rt_event_send(olpc_data->event, EVENT_SRCSAVER_ENTER);
}

/**
 * exit screen protection hook.
 */
static rt_err_t olpc_block_screen_protection_hook(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    rt_event_send(olpc_data->event, EVENT_SRCSAVER_EXIT);

    return RT_EOK;
}

static void olpc_block_screen_timer_start(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

#if defined(RT_USING_PISCES_TOUCH)
    olpc_block_screen_touch_register(parameter);
#endif

    rt_timer_start(olpc_data->srctimer);
}

static void olpc_block_screen_timer_stop(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    rt_timer_stop(olpc_data->srctimer);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_block_screen_touch_unregister(parameter);
#endif
}

/**
 * start screen protection.
 */
static rt_err_t olpc_block_screen_protection_enter(void *parameter)
{
    olpc_block_screen_timer_stop(parameter);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_block_touch_unregister(parameter);
    olpc_touch_list_clear();
#endif

    olpc_srcprotect_app_init(olpc_block_screen_protection_hook, parameter);

    return RT_EOK;
}

/**
 * exit screen protection.
 */
static rt_err_t olpc_block_screen_protection_exit(void *parameter)
{
    struct olpc_block_data *olpc_data = (struct olpc_block_data *)parameter;

    olpc_block_lutset(olpc_data);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_block_touch_register(parameter);
#endif
    olpc_block_screen_timer_start(parameter);

    unsigned int i, j;
    for (j = 0; j < BLOCKGAME_SCOPE_ROW; j++)
    {
        for (i = 0; i < BLOCKGAME_SCOPE_COL; i++)
        {
            BlockScopeBufBk[j][i] = 0x55;
        }
    }

    olpc_data->cmd  = CMD_UPDATE_GAME | CMD_UPDATE_GAME_BK;
    olpc_data->cmd |= CMD_UPDATE_GAME_SCORE | CMD_UPDATE_GAME_HSCORE;
    olpc_data->cmd |= CMD_UPDATE_GAME_LEVEL | CMD_UPDATE_GAME_SPEED;
    olpc_data->cmd |= CMD_UPDATE_GAME_NEXT | CMD_UPDATE_GAME_BLOCK;
    if (BlockGameover)
    {
        olpc_data->cmd |= CMD_UPDATE_GAME_OVER;
    }
    rt_event_send(olpc_data->event, EVENT_GAME_PROCESS);

    olpc_data->cmd |= CMD_UPDATE_CTRL | CMD_UPDATE_CTRL_BK;
    olpc_data->cmd |= CMD_UPDATE_CTRL_PAUSE;
    rt_event_send(olpc_data->event, EVENT_DISPLAY_REFRESH);

    return RT_EOK;
}
#endif

/*
 **************************************************************************************************
 *
 * olpc block demo init & thread
 *
 **************************************************************************************************
 */

/**
 * olpc block dmeo thread.
 */
static void olpc_block_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_block_data *olpc_data;

    olpc_data = (struct olpc_block_data *)rt_malloc(sizeof(struct olpc_block_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_block_data));

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_block_lutset(olpc_data);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->event = rt_event_create("block_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->event != RT_NULL);

    ret = olpc_block_init(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_block_touch_register(olpc_data);
#endif

#if defined(OLPC_APP_SRCSAVER_ENABLE)
    olpc_data->srctimer = rt_timer_create("blockprotect",
                                          olpc_block_srcprotect_timerISR,
                                          (void *)olpc_data,
                                          BLOCK_SRCSAVER_TIME,
                                          RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->srctimer != RT_NULL);
    olpc_block_screen_timer_start(olpc_data);
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
            ret = olpc_block_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        if (event & EVENT_SRCSAVER_ENTER)
        {
            ret = olpc_block_screen_protection_enter(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_SRCSAVER_EXIT)
        {
            ret = olpc_block_screen_protection_exit(olpc_data);
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
    olpc_block_touch_unregister(olpc_data);
    olpc_touch_list_clear();
#endif

    olpc_block_deinit(olpc_data);

    rt_event_delete(olpc_data->event);
    olpc_data->event = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;

    rt_event_send(olpc_main_event, EVENT_APP_CLOCK);
}

/**
 * olpc block demo application init.
 */
#if defined(OLPC_DLMODULE_ENABLE)
SECTION(".param") rt_uint16_t dlmodule_thread_priority = 5;
SECTION(".param") rt_uint32_t dlmodule_thread_stacksize = 2048;
int main(int argc, char *argv[])
{
    olpc_block_thread(RT_NULL);
    return RT_EOK;
}

#else
int olpc_block_app_init(void)
{
    rt_thread_t rtt_block;

    rtt_block = rt_thread_create("block", olpc_block_thread, RT_NULL, 2048 * 2, 5, 10);
    RT_ASSERT(rtt_block != RT_NULL);
    rt_thread_startup(rtt_block);

    return RT_EOK;
}
//INIT_APP_EXPORT(olpc_block_app_init);
#endif

#endif
