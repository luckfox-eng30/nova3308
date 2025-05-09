/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef UI_MAP_368X448_H
#define UI_MAP_368X448_H

/* Main region */
#define DISP_XRES               368
#define DISP_YRES               448

/* Touch region */
#define TOUCH_REGION_X          0
#define TOUCH_REGION_Y          0
#define TOUCH_REGION_W          DISP_XRES
#define TOUCH_REGION_H          DISP_YRES

/* Clock win */
#define CLOCK_REGION_X          0
#define CLOCK_REGION_Y          0

#define CLOCK_WIN_COLOR_DEPTH   16
#define CLOCK_WIN_XRES          DISP_XRES
#define CLOCK_WIN_YRES          DISP_YRES
#define CLOCK_HOR_PAGE_MAX      4

#define CLOCK_MAX_BUF_ID        CLOCK_HOR_PAGE_MAX
#define CLOCK_PAGE_FOC_ID       1

#define CLOCK_WIN_FB_W          (CLOCK_WIN_XRES * CLOCK_MAX_BUF_ID)
#define CLOCK_WIN_FB_H          CLOCK_WIN_YRES

/* Message win */
#define MSG_REGION_X            0
#define MSG_REGION_Y            0

#define MSG_WIN_XRES            DISP_XRES
#define MSG_WIN_YRES            DISP_YRES
#define MSG_WIN_COLOR_DEPTH     16

#define MSG_WIN_FB_W            MSG_WIN_XRES
#define MSG_WIN_FB_H            (MSG_WIN_YRES * 2)

/* LOGO max w,h: 160,160. min w,h: 64,64*/
#define MSG_LOGO_BIG_W          160
#define MSG_LOGO_BIG_H          160
#define MSG_LOGO_SMALL_W        64
#define MSG_LOGO_SMALL_H        64
#define MSG_LOGO_Y_START        180
#define MSG_LOGO_Y_MID          120
#define MSG_LOGO_Y_FINAL_REL    30
#define MSG_NAME_Y_START        264
#define MSG_NAME_Y_MID          285
#define MSG_NAME_Y_FINAL_REL    100
#define MSG_TIME_Y_FINAL_REL    140
#define MSG_CONTENT_Y_FINAL     193
#define MSG_BUF_OFFSET          (MSG_WIN_YRES - MSG_TIME_Y_FINAL_REL)
#define MSG_LOGO_Y_FINAL        (MSG_LOGO_Y_FINAL_REL + MSG_BUF_OFFSET)
#define MSG_NAME_Y_FINAL        (MSG_NAME_Y_FINAL_REL + MSG_BUF_OFFSET)
#define MSG_ANIM_STEP           16

/* Charging win */
#define CHARGING_WIN_XRES          368
#define CHARGING_WIN_YRES          DISP_YRES
#define CHARGING_WIN_FB_W          RT_ALIGN(CHARGING_WIN_XRES, 4)
#define CHARGING_WIN_FB_H          CHARGING_WIN_YRES
#define CHARGING_WIN_COLOR_DEPTH   8
#define CHARGING_ANIM_STEP         19
#define CHARGING_ANIM_LOOP_START   4

#define CHARGING_REGION_X          0
#define CHARGING_REGION_Y          ((DISP_YRES - CHARGING_WIN_YRES) / 2)

/* Alpha win */
#define ALPHA_REGION_X          0
#define ALPHA_REGION_Y          0

#define ALPHA_WIN_XRES          DISP_XRES
#define ALPHA_WIN_YRES          DISP_YRES
#define ALPHA_WIN_FB_W          ALPHA_WIN_XRES
#define ALPHA_WIN_FB_H          ALPHA_WIN_YRES
#define ALPHA_WIN_COLOR_DEPTH   32

/* Dialog box */
#define DIALOG_REGION_X         0
#define DIALOG_REGION_Y         0

#define DIALOG_WIN_XRES         DISP_XRES
#define DIALOG_WIN_YRES         DISP_YRES
#define DIALOG_WIN_FB_W         DIALOG_WIN_XRES
#define DIALOG_WIN_FB_H         DIALOG_WIN_YRES
#define DIALOG_WIN_COLOR_DEPTH  16

/* Funclist win */
#define FUNC_REGION_X           0
#define FUNC_REGION_Y           0

#define FUNC_HOR_PAGE_MAX       2
#define FUNC_WIN_XRES           DISP_XRES
#define FUNC_WIN_YRES           DISP_YRES
#define FUNC_WIN_FB_W           (FUNC_WIN_XRES * FUNC_HOR_PAGE_MAX)
#define FUNC_WIN_FB_H           FUNC_WIN_YRES
#define FUNC_WIN_COLOR_DEPTH    16

#define FUNC_ICON_HOR_NUM       2
#define FUNC_ICON_VER_NUM       3
#define FUNC_ICON_W             128
#define FUNC_ICON_H             128
#define FUNC_ICON_LEFT_PADDING  0
#define FUNC_ICON_RIGHT_PADDING 0

#define FUNC_ANIM_STEP          5
#define FUNC_ANIM_START_W       128
#define FUNC_ANIM_START_H       128

/* Setting menu win */
#define MENU_REGION_X           0
#define MENU_REGION_Y           0

#define MENU_WIN_XRES           DISP_XRES
#define MENU_WIN_YRES           DISP_YRES
#define MENU_WIN_FB_W           MENU_WIN_XRES
#define MENU_WIN_FB_H           (MENU_WIN_YRES * 2 + MENU_WIN_YRES * 2 / 3)
#define MENU_WIN_COLOR_DEPTH    16

/* Music page */
#define MUSIC_PIC_W             240
#define MUSIC_PIC_H             240
#define MUSIC_PIC_X             64
#define MUSIC_PIC_Y             15

#define MUSIC_NAME_W            DISP_XRES
#define MUSIC_NAME_H            33
#define MUSIC_NAME_X            0
#define MUSIC_NAME_Y            265

#define MUSIC_MODE_W            32
#define MUSIC_MODE_H            32
#define MUSIC_MODE_X            35
#define MUSIC_MODE_Y            305

#define MUSIC_VOL_W             32
#define MUSIC_VOL_H             32
#define MUSIC_VOL_X             ((DISP_XRES - MUSIC_VOL_W) / 2)
#define MUSIC_VOL_Y             305

#define MUSIC_MORE_W            32
#define MUSIC_MORE_H            32
#define MUSIC_MORE_X            (DISP_XRES - MUSIC_MODE_X - MUSIC_MORE_W)
#define MUSIC_MORE_Y            305

#define MUSIC_PREV_W            32
#define MUSIC_PREV_H            32
#define MUSIC_PREV_X            35
#define MUSIC_PREV_Y            374

#define MUSIC_PLAY_W            86
#define MUSIC_PLAY_H            86
#define MUSIC_PLAY_X            ((DISP_XRES - MUSIC_PLAY_W) / 2)
#define MUSIC_PLAY_Y            347

#define MUSIC_NEXT_W            32
#define MUSIC_NEXT_H            32
#define MUSIC_NEXT_X            (DISP_XRES - MUSIC_PREV_X - MUSIC_NEXT_W)
#define MUSIC_NEXT_Y            374

#define MUSIC_PREV_TOUCH_X      (MUSIC_PREV_X - (MUSIC_PLAY_W - MUSIC_PREV_W) / 2)
#define MUSIC_PREV_TOUCH_Y      MUSIC_PLAY_Y
#define MUSIC_PREV_TOUCH_W      MUSIC_PLAY_W
#define MUSIC_PREV_TOUCH_H      MUSIC_PLAY_H

#define MUSIC_NEXT_TOUCH_X      (MUSIC_NEXT_X - (MUSIC_PLAY_W - MUSIC_NEXT_W) / 2)
#define MUSIC_NEXT_TOUCH_Y      MUSIC_PLAY_Y
#define MUSIC_NEXT_TOUCH_W      MUSIC_PLAY_W
#define MUSIC_NEXT_TOUCH_H      MUSIC_PLAY_H

/* Motion win */
#define MOTION_WIN_XRES         DISP_XRES
#define MOTION_WIN_YRES         DISP_YRES
#define MOTION_WIN_FB_W         MOTION_WIN_XRES
#define MOTION_WIN_FB_H         MOTION_WIN_YRES
#define MOTION_WIN_COLOR_DEPTH  16

#define MOTION_ICONS_X          100
#define MOTION_LABEL_X          135

/* Weather win */
#define WEATHER_WIN_XRES        DISP_XRES
#define WEATHER_WIN_YRES        DISP_YRES
#define WEATHER_WIN_FB_W        WEATHER_WIN_XRES
#define WEATHER_WIN_FB_H        WEATHER_WIN_YRES
#define WEATHER_WIN_COLOR_DEPTH 16

/* Preview win */
#define PREVIEW_REGION_X        0
#define PREVIEW_REGION_Y        0

#define PREVIEW_WIN_COLOR_DEPTH 16
#define PREVIEW_WIN_XRES        DISP_XRES
#define PREVIEW_WIN_YRES        258
#define PREVIEW_GAP             55

#define PREVIEW_WIN_SHOW_X      258
#define PREVIEW_WIN_SHOW_Y      258
#define PREVIEW_WIN_FB_W        1308
#define PREVIEW_WIN_FB_H        PREVIEW_WIN_YRES

/* Heartrate win */
#define HEARTRATE_CHART_X       30
#define HEARTRATE_CHART_Y       200
#define HEARTRATE_CHART_H       140
#define HEARTRATE_CHART_W       280

#define HEARTRATE_RISE_ICON_X   50
#define HEARTRATE_RISE_ICON_Y   120
#define HEARTRATE_RISE_ICON_W   16
#define HEARTRATE_RISE_ICON_H   26

#define HEARTRATE_FALL_ICON_X   250
#define HEARTRATE_FALL_ICON_Y   120
#define HEARTRATE_FALL_ICON_W   16
#define HEARTRATE_FALL_ICON_H   26

#define HEARTRATE_ICON_X        40
#define HEARTRATE_ICON_Y        40

#endif
