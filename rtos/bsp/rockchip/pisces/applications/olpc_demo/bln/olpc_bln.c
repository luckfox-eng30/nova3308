/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLN_ENABLE)

#include "drv_heap.h"
#include "applications/common/image_info.h"
#include "applications/common/olpc_display.h"
#include "applications/common/olpc_ap.h"

#if defined(RT_USING_PISCES_TOUCH)
#include "drv_touch.h"
#include "applications/common/olpc_touch.h"
#endif

#define OLPC_DSP_BLN_REFRESH_ENABLE

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */
#define BLN_ARGB_BLUE_WIN     0
#define BLN_ARGB_CYAN_WIN     1

#define BLN_REGION_X          0
#define BLN_REGION_Y          0
#define BLN_REGION_W          WIN_LAYERS_W
#define BLN_REGION_H          WIN_LAYERS_H
#define BLN_FB_W              192 //162 * 2 ** (1/2)            /* bln frame buffer w */
#define BLN_FB_H              192 //162 * 2 ** (1/2)            /* bln frame buffer h */

/* Event define */
#define EVENT_BLN_REFRESH     (0x01UL << 0)
#define EVENT_BLN_EXIT        (0x01UL << 1)

/* Command define */
#define UPDATE_BLN            (0x01UL << 0)

#define BLN_ROTATE_TICKS        ((RT_TICK_PER_SECOND / 1000) * 33)
#define OLPC_ROTATE_PARAM_STEP  2 // 33/16.6667
#define BLN_ROTATE_TIME         900
#define BLN_BREADTH_TIME        3000

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern image_info_t bln_blue_info;
extern image_info_t bln_cyan_info;
/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_rotate_parms
{
    rt_uint8_t  alpha;
    rt_uint16_t cyan_rotate;
    rt_uint16_t blue_rotate;
};

struct olpc_bln_data
{
    rt_display_data_t disp;
    rt_timer_t        timer;

    rt_uint8_t *fb;
    rt_uint32_t fblen;

    rt_uint8_t *fb1;
    rt_uint32_t fblen1;

    rt_event_t  disp_event;
    rt_uint32_t cmd;

    rt_uint16_t rotatenum;
    rt_uint16_t rotatemax;
};

/* rotate parms test data
//0
02-19 14:57:52.611  2806  2806 D cja-bug : tow bitmap fade in alpha=0.0
02-19 14:57:52.612  2806  2806 D cja-bug : @@@ cyan rotation =0.0
02-19 14:57:52.612  2806  2806 D cja-bug : ### blue rotation =0.0
//1
02-19 14:57:52.632  2806  2806 D cja-bug : tow bitmap fade in alpha=0.0
02-19 14:57:52.632  2806  2806 D cja-bug : @@@ cyan rotation =0.0
02-19 14:57:52.632  2806  2806 D cja-bug : ### blue rotation =0.0
//3
02-19 14:57:52.643  2806  2806 D cja-bug : tow bitmap fade in alpha=0.0048180195
02-19 14:57:52.643  2806  2806 D cja-bug : @@@ cyan rotation =11.939652
02-19 14:57:52.643  2806  2806 D cja-bug : ### blue rotation =8.246894
//4
02-19 14:57:52.660  2806  2806 D cja-bug : tow bitmap fade in alpha=0.020068128
02-19 14:57:52.660  2806  2806 D cja-bug : @@@ cyan rotation =25.794107
02-19 14:57:52.660  2806  2806 D cja-bug : ### blue rotation =18.248
//5
02-19 14:57:52.676  2806  2806 D cja-bug : tow bitmap fade in alpha=0.045384955
02-19 14:57:52.677  2806  2806 D cja-bug : @@@ cyan rotation =41.798073
02-19 14:57:52.677  2806  2806 D cja-bug : ### blue rotation =30.041164
//6
02-19 14:57:52.693  2806  2806 D cja-bug : tow bitmap fade in alpha=0.086455435
02-19 14:57:52.693  2806  2806 D cja-bug : @@@ cyan rotation =60.461914
02-19 14:57:52.694  2806  2806 D cja-bug : ### blue rotation =44.84074
//7
02-19 14:57:52.710  2806  2806 D cja-bug : tow bitmap fade in alpha=0.1426237
02-19 14:57:52.710  2806  2806 D cja-bug : @@@ cyan rotation =80.519485
02-19 14:57:52.710  2806  2806 D cja-bug : ### blue rotation =60.845867
//8
02-19 14:57:52.726  2806  2806 D cja-bug : tow bitmap fade in alpha=0.20609082
02-19 14:57:52.726  2806  2806 D cja-bug : @@@ cyan rotation =100.66469
02-19 14:57:52.727  2806  2806 D cja-bug : ### blue rotation =77.07606
//9
02-19 14:57:52.743  2806  2806 D cja-bug : tow bitmap fade in alpha=0.2816702
02-19 14:57:52.743  2806  2806 D cja-bug : @@@ cyan rotation =122.36515
02-19 14:57:52.743  2806  2806 D cja-bug : ### blue rotation =94.56291
//10
02-19 14:57:52.761  2806  2806 D cja-bug : tow bitmap fade in alpha=0.35622072
02-19 14:57:52.762  2806  2806 D cja-bug : @@@ cyan rotation =143.07336
02-19 14:57:52.762  2806  2806 D cja-bug : ### blue rotation =111.09023
//11
02-19 14:57:52.776  2806  2806 D cja-bug : tow bitmap fade in alpha=0.43469378
02-19 14:57:52.776  2806  2806 D cja-bug : @@@ cyan rotation =165.08717
02-19 14:57:52.776  2806  2806 D cja-bug : ### blue rotation =127.83789
//12
02-19 14:57:52.793  2806  2806 D cja-bug : tow bitmap fade in alpha=0.5095385
02-19 14:57:52.793  2806  2806 D cja-bug : @@@ cyan rotation =186.35449
02-19 14:57:52.793  2806  2806 D cja-bug : ### blue rotation =144.21907
//13
02-19 14:57:52.809  2806  2806 D cja-bug : tow bitmap fade in alpha=0.57356197
02-19 14:57:52.809  2806  2806 D cja-bug : @@@ cyan rotation =206.3431
02-19 14:57:52.809  2806  2806 D cja-bug : ### blue rotation =158.45532
//14
02-19 14:57:52.826  2806  2806 D cja-bug : tow bitmap fade in alpha=0.6344103
02-19 14:57:52.826  2806  2806 D cja-bug : @@@ cyan rotation =226.37741
02-19 14:57:52.826  2806  2806 D cja-bug : ### blue rotation =173.34972
//15
02-19 14:57:52.843  2806  2806 D cja-bug : tow bitmap fade in alpha=0.6879297
02-19 14:57:52.843  2806  2806 D cja-bug : @@@ cyan rotation =244.8973
02-19 14:57:52.843  2806  2806 D cja-bug : ### blue rotation =185.6912
//16
02-19 14:57:52.859  2806  2806 D cja-bug : tow bitmap fade in alpha=0.735031
02-19 14:57:52.859  2806  2806 D cja-bug : @@@ cyan rotation =263.8637
02-19 14:57:52.859  2806  2806 D cja-bug : ### blue rotation =198.80405
//17
02-19 14:57:52.876  2806  2806 D cja-bug : tow bitmap fade in alpha=0.7789709
02-19 14:57:52.877  2806  2806 D cja-bug : @@@ cyan rotation =281.4878
02-19 14:57:52.877  2806  2806 D cja-bug : ### blue rotation =210.47105
//18
02-19 14:57:52.892  2806  2806 D cja-bug : tow bitmap fade in alpha=0.8133377
02-19 14:57:52.892  2806  2806 D cja-bug : @@@ cyan rotation =298.07516
02-19 14:57:52.893  2806  2806 D cja-bug : ### blue rotation =220.8226
//19
02-19 14:57:52.909  2806  2806 D cja-bug : tow bitmap fade in alpha=0.8486151
02-19 14:57:52.909  2806  2806 D cja-bug : @@@ cyan rotation =314.22256
02-19 14:57:52.909  2806  2806 D cja-bug : ### blue rotation =231.82112
//20
02-19 14:57:52.925  2806  2806 D cja-bug : tow bitmap fade in alpha=0.8760138
02-19 14:57:52.926  2806  2806 D cja-bug : @@@ cyan rotation =328.6374
02-19 14:57:52.926  2806  2806 D cja-bug : ### blue rotation =240.53479
//21
02-19 14:57:52.942  2806  2806 D cja-bug : tow bitmap fade in alpha=0.90065646
02-19 14:57:52.942  2806  2806 D cja-bug : @@@ cyan rotation =343.9531
02-19 14:57:52.942  2806  2806 D cja-bug : ### blue rotation =249.49344
//22
02-19 14:57:52.958  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9207489
02-19 14:57:52.958  2806  2806 D cja-bug : @@@ cyan rotation =357.0637
02-19 14:57:52.959  2806  2806 D cja-bug : ### blue rotation =257.9251
//23
02-19 14:57:52.975  2806  2806 D cja-bug : tow bitmap fade in alpha=0.93940675
02-19 14:57:52.975  2806  2806 D cja-bug : @@@ cyan rotation =369.9888
02-19 14:57:52.975  2806  2806 D cja-bug : ### blue rotation =265.90833
//24
02-19 14:57:52.992  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9552115
02-19 14:57:52.992  2806  2806 D cja-bug : @@@ cyan rotation =382.91388
02-19 14:57:52.992  2806  2806 D cja-bug : ### blue rotation =272.98907
//25
02-19 14:57:53.008  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9668419
02-19 14:57:53.008  2806  2806 D cja-bug : @@@ cyan rotation =394.76248
02-19 14:57:53.008  2806  2806 D cja-bug : ### blue rotation =279.65332
//26
02-19 14:57:53.025  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9776528
02-19 14:57:53.025  2806  2806 D cja-bug : @@@ cyan rotation =405.32904
02-19 14:57:53.025  2806  2806 D cja-bug : ### blue rotation =286.73407
//27
02-19 14:57:53.041  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9853652
02-19 14:57:53.041  2806  2806 D cja-bug : @@@ cyan rotation =415.27405
02-19 14:57:53.042  2806  2806 D cja-bug : ### blue rotation =292.31195
//28
02-19 14:57:53.058  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9915448
02-19 14:57:53.058  2806  2806 D cja-bug : @@@ cyan rotation =425.84067
02-19 14:57:53.058  2806  2806 D cja-bug : ### blue rotation =297.70868
//29
02-19 14:57:53.075  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9962924
02-19 14:57:53.075  2806  2806 D cja-bug : @@@ cyan rotation =435.73193
02-19 14:57:53.075  2806  2806 D cja-bug : ### blue rotation =303.1054
//30
02-19 14:57:53.091  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9982965
02-19 14:57:53.092  2806  2806 D cja-bug : @@@ cyan rotation =443.55258
02-19 14:57:53.092  2806  2806 D cja-bug : ### blue rotation =308.1847
//31
02-19 14:57:53.108  2806  2806 D cja-bug : tow bitmap fade in alpha=0.9998207
02-19 14:57:53.108  2806  2806 D cja-bug : @@@ cyan rotation =451.862
02-19 14:57:53.108  2806  2806 D cja-bug : ### blue rotation =313.3255
//32
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.125  2806  2806 D cja-bug : @@@ cyan rotation =459.68268
02-19 14:57:53.125  2806  2806 D cja-bug : ### blue rotation =317.34366
//33
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.142  2806  2806 D cja-bug : @@@ cyan rotation =467.9921
02-19 14:57:53.142  2806  2806 D cja-bug : ### blue rotation =321.613
//34
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.158  2806  2806 D cja-bug : @@@ cyan rotation =474.32117
02-19 14:57:53.158  2806  2806 D cja-bug : ### blue rotation =325.36505
//35
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.174  2806  2806 D cja-bug : @@@ cyan rotation =480.14786
02-19 14:57:53.174  2806  2806 D cja-bug : ### blue rotation =328.72586
//36
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.191  2806  2806 D cja-bug : @@@ cyan rotation =486.33868
02-19 14:57:53.191  2806  2806 D cja-bug : ### blue rotation =332.24634
//37
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.207  2806  2806 D cja-bug : @@@ cyan rotation =492.16537
02-19 14:57:53.208  2806  2806 D cja-bug : ### blue rotation =334.9915
//38
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.224  2806  2806 D cja-bug : @@@ cyan rotation =498.27887
02-19 14:57:53.224  2806  2806 D cja-bug : ### blue rotation =337.90823
//39
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.241  2806  2806 D cja-bug : @@@ cyan rotation =502.71317
02-19 14:57:53.241  2806  2806 D cja-bug : ### blue rotation =340.55014
//40
02-19 14:57:53.124  2806  2806 D cja-bug : tow bitmap fade in alpha=1.0
02-19 14:57:53.257  2806  2806 D cja-bug : @@@ cyan rotation =507.4246
02-19 14:57:53.257  2806  2806 D cja-bug : ### blue rotation =342.85464
//41
02-19 14:57:53.274  2806  2806 D cja-bug : tow bitmap fade out alpha=0.9890855
02-19 14:57:53.274  2806  2806 D cja-bug : @@@ cyan rotation =511.75687
02-19 14:57:53.274  2806  2806 D cja-bug : ### blue rotation =345.15918
//42
02-19 14:57:53.290  2806  2806 D cja-bug : tow bitmap fade out alpha=0.9468218
02-19 14:57:53.290  2806  2806 D cja-bug : @@@ cyan rotation =515.3034
02-19 14:57:53.290  2806  2806 D cja-bug : ### blue rotation =347.25687
//43
02-19 14:57:53.307  2806  2806 D cja-bug : tow bitmap fade out alpha=0.8645937
02-19 14:57:53.307  2806  2806 D cja-bug : @@@ cyan rotation =519.07153
02-19 14:57:53.307  2806  2806 D cja-bug : ### blue rotation =348.98846
//44
02-19 14:57:53.324  2806  2806 D cja-bug : tow bitmap fade out alpha=0.7600328
02-19 14:57:53.324  2806  2806 D cja-bug : @@@ cyan rotation =522.1249
02-19 14:57:53.324  2806  2806 D cja-bug : ### blue rotation =350.6182
//45
02-19 14:57:53.340  2806  2806 D cja-bug : tow bitmap fade out alpha=0.6313456
02-19 14:57:53.340  2806  2806 D cja-bug : @@@ cyan rotation =524.98914
02-19 14:57:53.340  2806  2806 D cja-bug : ### blue rotation =352.3498
//46
02-19 14:57:53.357  2806  2806 D cja-bug : tow bitmap fade out alpha=0.5033919
02-19 14:57:53.357  2806  2806 D cja-bug : @@@ cyan rotation =527.8534
02-19 14:57:53.357  2806  2806 D cja-bug : ### blue rotation =353.61133
//47
02-19 14:57:53.373  2806  2806 D cja-bug : tow bitmap fade out alpha=0.39532864
02-19 14:57:53.373  2806  2806 D cja-bug : @@@ cyan rotation =530.0344
02-19 14:57:53.373  2806  2806 D cja-bug : ### blue rotation =354.7364
//48
02-19 14:57:53.390  2806  2806 D cja-bug : tow bitmap fade out alpha=0.29743338
02-19 14:57:53.390  2806  2806 D cja-bug : @@@ cyan rotation =532.03345
02-19 14:57:53.390  2806  2806 D cja-bug : ### blue rotation =355.93176
//49
02-19 14:57:53.406  2806  2806 D cja-bug : tow bitmap fade out alpha=0.22051585
02-19 14:57:53.407  2806  2806 D cja-bug : @@@ cyan rotation =533.91486
02-19 14:57:53.407  2806  2806 D cja-bug : ### blue rotation =356.87732
//50
02-19 14:57:53.423  2806  2806 D cja-bug : tow bitmap fade out alpha=0.15368539
02-19 14:57:53.423  2806  2806 D cja-bug : @@@ cyan rotation =535.5124
02-19 14:57:53.423  2806  2806 D cja-bug : ### blue rotation =357.57062
//51
02-19 14:57:53.440  2806  2806 D cja-bug : tow bitmap fade out alpha=0.10260898
02-19 14:57:53.440  2806  2806 D cja-bug : @@@ cyan rotation =536.6839
02-19 14:57:53.440  2806  2806 D cja-bug : ### blue rotation =358.26392
//52
02-19 14:57:53.456  2806  2806 D cja-bug : tow bitmap fade out alpha=0.064938664
02-19 14:57:53.456  2806  2806 D cja-bug : @@@ cyan rotation =537.78644
02-19 14:57:53.456  2806  2806 D cja-bug : ### blue rotation =358.91644
//53
02-19 14:57:53.473  2806  2806 D cja-bug : tow bitmap fade out alpha=0.036452174
02-19 14:57:53.473  2806  2806 D cja-bug : @@@ cyan rotation =538.7915
02-19 14:57:53.473  2806  2806 D cja-bug : ### blue rotation =359.29175
//54
02-19 14:57:53.489  2806  2806 D cja-bug : tow bitmap fade out alpha=0.0177145
02-19 14:57:53.489  2806  2806 D cja-bug : @@@ cyan rotation =539.1496
02-19 14:57:53.490  2806  2806 D cja-bug : ### blue rotation =359.50162
//55
02-19 14:57:53.506  2806  2806 D cja-bug : tow bitmap fade out alpha=0.0056108832
02-19 14:57:53.506  2806  2806 D cja-bug : @@@ cyan rotation =539.53
02-19 14:57:53.506  2806  2806 D cja-bug : ### blue rotation =359.72458
//56
02-19 14:57:53.523  2806  2806 D cja-bug : tow bitmap fade out alpha=7.1525574E-4
02-19 14:57:53.523  2806  2806 D cja-bug : @@@ cyan rotation =539.91046
02-19 14:57:53.523  2806  2806 D cja-bug : ### blue rotation =359.94754
//57
02-19 14:57:53.539  2806  2806 D cja-bug : tow bitmap fade out alpha=0.0
02-19 14:57:53.539  2806  2806 D cja-bug : @@@ cyan rotation =540.0
02-19 14:57:53.539  2806  2806 D cja-bug : ### blue rotation =360.0
*/
static struct olpc_rotate_parms bln_rotate_valtab[] =
{
    //{(rt_uint8_t)(0.0 * 255),            (rt_uint16_t)(0.0),             (rt_uint16_t)(0.0),      },   //0
    {(rt_uint8_t)(0.0 * 255), (rt_uint16_t)(0.0), (rt_uint16_t)(0.0),      },                          //1
    {(rt_uint8_t)(0.0048180195 * 255), (rt_uint16_t)(11.939652), (rt_uint16_t)(8.246894), },           //2
    {(rt_uint8_t)(0.020068128 * 255), (rt_uint16_t)(25.794107), (rt_uint16_t)(18.248),   },            //3
    {(rt_uint8_t)(0.045384955 * 255), (rt_uint16_t)(41.798073), (rt_uint16_t)(30.041164),},            //4
    {(rt_uint8_t)(0.086455435 * 255), (rt_uint16_t)(60.461914), (rt_uint16_t)(44.84074), },            //5
    {(rt_uint8_t)(0.1426237 * 255), (rt_uint16_t)(80.519485), (rt_uint16_t)(60.845867),},              //6
    {(rt_uint8_t)(0.20609082 * 255), (rt_uint16_t)(100.66469), (rt_uint16_t)(77.07606), },             //7
    {(rt_uint8_t)(0.2816702 * 255), (rt_uint16_t)(122.36515), (rt_uint16_t)(94.56291), },              //8
    {(rt_uint8_t)(0.35622072 * 255), (rt_uint16_t)(143.07336), (rt_uint16_t)(111.09023),},             //9
    {(rt_uint8_t)(0.43469378 * 255), (rt_uint16_t)(165.08717), (rt_uint16_t)(127.83789),},             //10
    {(rt_uint8_t)(0.5095385 * 255), (rt_uint16_t)(186.35449), (rt_uint16_t)(144.21907),},              //11
    {(rt_uint8_t)(0.57356197 * 255), (rt_uint16_t)(206.3431), (rt_uint16_t)(158.45532),},              //12
    {(rt_uint8_t)(0.6344103 * 255), (rt_uint16_t)(226.37741), (rt_uint16_t)(173.34972),},              //13
    {(rt_uint8_t)(0.6879297 * 255), (rt_uint16_t)(244.8973), (rt_uint16_t)(185.6912), },               //14
    {(rt_uint8_t)(0.735031 * 255), (rt_uint16_t)(263.8637), (rt_uint16_t)(198.80405),},                //15
    {(rt_uint8_t)(0.7789709 * 255), (rt_uint16_t)(281.4878), (rt_uint16_t)(210.47105),},               //16
    {(rt_uint8_t)(0.8133377 * 255), (rt_uint16_t)(298.07516), (rt_uint16_t)(220.8226), },              //17
    {(rt_uint8_t)(0.8486151 * 255), (rt_uint16_t)(314.22256), (rt_uint16_t)(231.82112),},              //18
    {(rt_uint8_t)(0.8760138 * 255), (rt_uint16_t)(328.6374), (rt_uint16_t)(240.53479),},               //19
    {(rt_uint8_t)(0.90065646 * 255), (rt_uint16_t)(343.9531), (rt_uint16_t)(249.49344),},              //20
    {(rt_uint8_t)(0.9207489 * 255), (rt_uint16_t)(357.0637), (rt_uint16_t)(257.9251), },               //21
    {(rt_uint8_t)(0.93940675 * 255), (rt_uint16_t)(369.9888), (rt_uint16_t)(265.90833),},              //22
    {(rt_uint8_t)(0.9552115 * 255), (rt_uint16_t)(382.91388), (rt_uint16_t)(272.98907),},              //23
    {(rt_uint8_t)(0.9668419 * 255), (rt_uint16_t)(394.76248), (rt_uint16_t)(279.65332),},              //24
    {(rt_uint8_t)(0.9776528 * 255), (rt_uint16_t)(405.32904), (rt_uint16_t)(286.73407),},              //25
    {(rt_uint8_t)(0.9853652 * 255), (rt_uint16_t)(415.27405), (rt_uint16_t)(292.31195),},              //26
    {(rt_uint8_t)(0.9915448 * 255), (rt_uint16_t)(425.84067), (rt_uint16_t)(297.70868),},              //27
    {(rt_uint8_t)(0.9962924 * 255), (rt_uint16_t)(435.73193), (rt_uint16_t)(303.1054), },              //28
    {(rt_uint8_t)(0.9982965 * 255), (rt_uint16_t)(443.55258), (rt_uint16_t)(308.1847), },              //29
    {(rt_uint8_t)(0.9998207 * 255), (rt_uint16_t)(451.862), (rt_uint16_t)(313.3255), },                //30
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(459.68268), (rt_uint16_t)(317.34366),},                    //31
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(467.9921), (rt_uint16_t)(321.613),  },                     //32
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(474.32117), (rt_uint16_t)(325.36505),},                    //33
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(480.14786), (rt_uint16_t)(328.72586),},                    //34
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(486.33868), (rt_uint16_t)(332.24634),},                    //35
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(492.16537), (rt_uint16_t)(334.9915), },                    //36
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(498.27887), (rt_uint16_t)(337.90823),},                    //37
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(502.71317), (rt_uint16_t)(340.55014),},                    //38
    {(rt_uint8_t)(1.0 * 255), (rt_uint16_t)(507.4246), (rt_uint16_t)(342.85464),},                     //39
    {(rt_uint8_t)(0.9890855 * 255), (rt_uint16_t)(511.75687), (rt_uint16_t)(345.15918),},              //40
    {(rt_uint8_t)(0.9468218 * 255), (rt_uint16_t)(515.3034), (rt_uint16_t)(347.25687),},               //41
    {(rt_uint8_t)(0.8645937 * 255), (rt_uint16_t)(519.07153), (rt_uint16_t)(348.98846),},              //42
    {(rt_uint8_t)(0.7600328 * 255), (rt_uint16_t)(522.1249), (rt_uint16_t)(350.6182), },               //43
    {(rt_uint8_t)(0.6313456 * 255), (rt_uint16_t)(524.98914), (rt_uint16_t)(352.3498), },              //44
    {(rt_uint8_t)(0.5033919 * 255), (rt_uint16_t)(527.8534), (rt_uint16_t)(353.61133),},               //45
    {(rt_uint8_t)(0.39532864 * 255), (rt_uint16_t)(530.0344), (rt_uint16_t)(354.7364), },              //46
    {(rt_uint8_t)(0.29743338 * 255), (rt_uint16_t)(532.03345), (rt_uint16_t)(355.93176),},             //47
    {(rt_uint8_t)(0.22051585 * 255), (rt_uint16_t)(533.91486), (rt_uint16_t)(356.87732),},             //48
    {(rt_uint8_t)(0.15368539 * 255), (rt_uint16_t)(535.5124), (rt_uint16_t)(357.57062),},              //49
    {(rt_uint8_t)(0.10260898 * 255), (rt_uint16_t)(536.6839), (rt_uint16_t)(358.26392),},              //50
    {(rt_uint8_t)(0.064938664 * 255), (rt_uint16_t)(537.78644), (rt_uint16_t)(358.91644),},            //51
    {(rt_uint8_t)(0.036452174 * 255), (rt_uint16_t)(538.7915), (rt_uint16_t)(359.29175),},             //52
    {(rt_uint8_t)(0.0177145 * 255), (rt_uint16_t)(539.1496), (rt_uint16_t)(359.50162),},               //53
    {(rt_uint8_t)(0.0056108832 * 255), (rt_uint16_t)(539.53), (rt_uint16_t)(359.72458),},              //54
    {(rt_uint8_t)(0.0 * 255), (rt_uint16_t)(539.91046), (rt_uint16_t)(359.94754),},                    //55
    {(rt_uint8_t)(0.0 * 255), (rt_uint16_t)(540.0), (rt_uint16_t)(360.0),    },                        //56
};

/*
 **************************************************************************************************
 *
 * bln test demo
 *
 **************************************************************************************************
 */
#ifdef OLPC_DSP_BLN_REFRESH_ENABLE
#include "drv_dsp.h"
/**
 * olpc bln refresh page
 */
typedef struct _img_param_t
{
    uint32_t w;
    uint32_t h;
    uint32_t in_size;
    int  *in_buf;
    int *out_buf;
    uint32_t out_size;
    int angle;
    int dst_str;
    int xcen;
    int ycen;
} img_param_t;

#define DSP_ALGO_ROTATE_START   0x40000004
static struct dsp_work *dsp_create_work(uint32_t id,
                                        uint32_t algo_type,
                                        uint32_t param,
                                        uint32_t param_size)
{
    struct dsp_work *work;

    work = rk_dsp_work_create(RKDSP_ALGO_WORK);
    if (work)
    {
        work->id = id;
        work->algo_type = algo_type;
        work->param = param;
        work->param_size = param_size;
    }
    return work;
}

static void dsp_destory_work(struct dsp_work *work)
{
    rk_dsp_work_destroy(work);
}

struct dsp_work *g_work;
struct rt_device *g_dsp_dev;
void olpc_bln_dsp_init()
{
    struct img_param_t *param;

    g_dsp_dev = rt_device_find("dsp0");

    RT_ASSERT(g_dsp_dev != RT_NULL);
    rt_device_open(g_dsp_dev, RT_DEVICE_OFLAG_RDWR);

    /* Start to work */
    param = rkdsp_malloc(sizeof(img_param_t));
    memset(param, 0, sizeof(img_param_t));

    g_work = dsp_create_work(DSP_ALGO_ROTATE_START, DSP_ALGO_ROTATE_START,
                             (uint32_t)param, sizeof(img_param_t));
    if (!g_work)
    {
        rt_kprintf("dsp create config work fail\n");
    }
}

//static int printf_cnt = 0;
void dsp_bln_process(int angle, int w, int h, unsigned int *src, unsigned int *dst, int dst_str, int xcen, int ycen)
{
    img_param_t *param = (img_param_t *)g_work->param;
    rt_err_t ret;

    //rt_kprintf("angle = %d, w = %d, h = %d, src = 0x%08x, dst = 0x%08x, dst_str = %d, xcen = %d, ycen = %d\n",
    //            (int)angle, w, h, src, dst, dst_str, xcen, ycen);

    param->w = (uint16_t)w;
    param->h = (uint16_t)h;
    param->in_size = w * h * 4;
    param->in_buf = (int *)src;
    param->out_buf = (int *)dst;
    param->out_size = BLN_FB_W * BLN_FB_H * 4;
    param->angle = angle;
    param->dst_str = dst_str;
    param->xcen =  xcen;
    param->ycen = ycen;
    ret = rt_device_control(g_dsp_dev, RKDSP_CTL_QUEUE_WORK, g_work);
    RT_ASSERT(!ret);
    ret = rt_device_control(g_dsp_dev, RKDSP_CTL_DEQUEUE_WORK, g_work);
    RT_ASSERT(!ret);

#if 0
    printf_cnt++;
    if (printf_cnt < 10)
    {
        rt_kprintf("cycle = %d\n", g_work->cycles);
    }
#endif
}

static void olpc_bln_dsp_deinit(void)
{
    rkdsp_free((char *)g_work->param);
    dsp_destory_work(g_work);
    rt_device_close(g_dsp_dev);
}
#endif

/*
 **************************************************************************************************
 *
 * bln test demo
 *
 **************************************************************************************************
 */

/**
 * olpc bln lut set.
 */
static rt_err_t olpc_bln_lutset(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut0, lut1;

    lut0.winId = BLN_ARGB_BLUE_WIN;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_ARGB888;
    lut0.lut  = RT_NULL;
    lut0.size = 0;

    lut1.winId = BLN_ARGB_CYAN_WIN;
    lut1.format = RTGRAPHIC_PIXEL_FORMAT_ARGB888;
    lut1.lut  = RT_NULL;
    lut1.size = 0;

    ret = rt_display_lutset(&lut0, &lut1, RT_NULL);
    RT_ASSERT(ret == RT_EOK);

    // clear screen
    {
        struct olpc_bln_data *olpc_data = (struct olpc_bln_data *)parameter;
        rt_device_t device = olpc_data->disp->device;
        struct rt_device_graphic_info info;

        ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
        RT_ASSERT(ret == RT_EOK);

        rt_display_win_clear(BLN_ARGB_BLUE_WIN, RTGRAPHIC_PIXEL_FORMAT_ARGB888, 0, WIN_LAYERS_H, 0);
    }

    return ret;
}

/**
 * olpc bln demo init.
 */
static rt_err_t olpc_bln_init(struct olpc_bln_data *olpc_data)
{
    rt_err_t    ret;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->fblen = BLN_FB_W * BLN_FB_H * 4;
    olpc_data->fb    = (rt_uint8_t *)rt_dma_malloc_large(olpc_data->fblen);
    RT_ASSERT(olpc_data->fb != RT_NULL);
    rt_memset(olpc_data->fb, 0, olpc_data->fblen);

    olpc_data->fblen1 = BLN_FB_W * BLN_FB_H * 4;
    olpc_data->fb1    = (rt_uint8_t *)rt_dma_malloc_large(olpc_data->fblen1);
    RT_ASSERT(olpc_data->fb1 != RT_NULL);
    rt_memset(olpc_data->fb1, 0, olpc_data->fblen1);

#ifdef OLPC_DSP_BLN_REFRESH_ENABLE
    olpc_bln_dsp_init();
#endif

    return RT_EOK;
}

/**
 * olpc bln demo deinit.
 */
static void olpc_bln_deinit(struct olpc_bln_data *olpc_data)
{
#ifdef OLPC_DSP_BLN_REFRESH_ENABLE
    olpc_bln_dsp_deinit();
#endif

    rt_dma_free_large((void *)olpc_data->fb1);
    olpc_data->fb1 = RT_NULL;

    rt_dma_free_large((void *)olpc_data->fb);
    olpc_data->fb = RT_NULL;
}

//static rt_uint32_t cnt = 0;
static rt_err_t olpc_bln_cyan_refresh(struct olpc_bln_data *olpc_data,
                                      struct rt_display_config *wincfg)
{
    rt_err_t ret;
    //rt_int32_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = BLN_ARGB_CYAN_WIN;
    wincfg->fb    = olpc_data->fb1;
    wincfg->w     = ((BLN_FB_W + 1) / 2) * 2;
    wincfg->h     = BLN_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h * 4;
    wincfg->x     = BLN_REGION_X + (BLN_REGION_W - BLN_FB_W) / 2;
    wincfg->y     = BLN_REGION_Y + (BLN_REGION_H - BLN_FB_H) / 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    wincfg->alphaEn = 1;
    wincfg->alphaMode = VOP_ALPHA_MODE_USER_DEFINED;
    wincfg->alphaPreMul = VOP_NON_PREMULT_ALPHA;
    wincfg->globalAlphaValue = bln_rotate_valtab[olpc_data->rotatenum].alpha;

    RT_ASSERT((wincfg->w % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen1);

    img_info = &bln_cyan_info;
    //rt_uint16_t rotate = bln_rotate_valtab[olpc_data->rotatenum].blue_rotate % 360;
    rt_uint16_t rotate = bln_rotate_valtab[olpc_data->rotatenum].cyan_rotate % 360;
    RT_ASSERT(img_info->w <= wincfg->w);
    RT_ASSERT(img_info->h <= wincfg->h);

    //rt_kprintf("num = %d, rotate = %d\n", olpc_data->rotatenum, rotate);
#if 0
    xoffset = (BLN_FB_W - img_info->w) / 2;
    yoffset = (BLN_FB_H - img_info->h) / 2;
    rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
#endif

    //rt_tick_t ticks = rt_tick_get();

#ifdef OLPC_DSP_BLN_REFRESH_ENABLE
    dsp_bln_process((float)(rotate),
                    img_info->w,
                    img_info->h,
                    (unsigned int *)img_info->data + 0x1c000000 / 4,
                    (unsigned int *)wincfg->fb + (BLN_FB_H + 1) * BLN_FB_W / 2,
                    wincfg->w,
                    img_info->w / 2,
                    img_info->h / 2);
#else
    rt_memset(wincfg->fb1, 0, wincfg->fblen1);
    rt_display_rotate_32bit((float)(rotate),
                            img_info->w,
                            img_info->h,
                            (unsigned int *)img_info->data,
                            (unsigned int *)wincfg->fb1 + (BLN_FB_H + 1) * BLN_FB_W / 2,
                            wincfg->w,
                            img_info->w / 2,
                            img_info->h / 2);
#endif

    //if (++cnt < 20) {
    //    rt_kprintf("ticks111 = %d\n", rt_tick_get() - ticks);
    //}

    return RT_EOK;
}

//static rt_uint32_t cnt = 0;
static rt_err_t olpc_bln_blue_refresh(struct olpc_bln_data *olpc_data,
                                      struct rt_display_config *wincfg)
{
    rt_err_t ret;
    //rt_int32_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = BLN_ARGB_BLUE_WIN;
    wincfg->fb    = olpc_data->fb;
    wincfg->w     = ((BLN_FB_W + 1) / 2) * 2;
    wincfg->h     = BLN_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h * 4;
    wincfg->x     = BLN_REGION_X + (BLN_REGION_W - BLN_FB_W) / 2;
    wincfg->y     = BLN_REGION_Y + (BLN_REGION_H - BLN_FB_H) / 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    wincfg->alphaEn = 1;
    wincfg->alphaMode = VOP_ALPHA_MODE_USER_DEFINED;
    wincfg->alphaPreMul = VOP_NON_PREMULT_ALPHA;
    wincfg->globalAlphaValue = bln_rotate_valtab[olpc_data->rotatenum].alpha;

    RT_ASSERT((wincfg->w % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    img_info = &bln_blue_info;
    rt_uint16_t rotate = bln_rotate_valtab[olpc_data->rotatenum].blue_rotate % 360;
    //rt_uint16_t rotate = bln_rotate_valtab[olpc_data->rotatenum].cyan_rotate % 360;
    RT_ASSERT(img_info->w <= wincfg->w);
    RT_ASSERT(img_info->h <= wincfg->h);

    //rt_kprintf("num = %d, rotate = %d\n", olpc_data->rotatenum, rotate);
#if 0
    xoffset = (BLN_FB_W - img_info->w) / 2;
    yoffset = (BLN_FB_H - img_info->h) / 2;
    rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
#endif

    //rt_tick_t ticks = rt_tick_get();

#ifdef OLPC_DSP_BLN_REFRESH_ENABLE
    dsp_bln_process((float)(rotate),
                    img_info->w,
                    img_info->h,
                    (unsigned int *)img_info->data + 0x1c000000 / 4,
                    (unsigned int *)wincfg->fb + (BLN_FB_H + 1) * BLN_FB_W / 2,
                    wincfg->w,
                    img_info->w / 2,
                    img_info->h / 2);
#else
    rt_memset(wincfg->fb, 0, wincfg->fblen);
    rt_display_rotate_32bit((float)(rotate),
                            img_info->w,
                            img_info->h,
                            (unsigned int *)img_info->data,
                            (unsigned int *)wincfg->fb + (BLN_FB_H + 1) * BLN_FB_W / 2,
                            wincfg->w,
                            img_info->w / 2,
                            img_info->h / 2);
#endif

    //if (++cnt < 20) {
    //    rt_kprintf("ticks111 = %d\n", rt_tick_get() - ticks);
    //}

    return RT_EOK;
}

/**
 * olpc bln refresh process
 */
static rt_err_t olpc_bln_task_fun(struct olpc_bln_data *olpc_data)
{
    rt_err_t ret;
    struct rt_display_config  wincfg0, wincfg1;
    struct rt_display_config *winhead = RT_NULL;

    //rt_tick_t ticks = rt_tick_get();

    rt_memset(&wincfg0, 0, sizeof(struct rt_display_config));
    rt_memset(&wincfg1, 0, sizeof(struct rt_display_config));
    wincfg0.zpos = WIN_BOTTOM_LAYER;
    wincfg1.zpos = WIN_TOP_LAYER;

    if ((olpc_data->cmd & UPDATE_BLN) == UPDATE_BLN)
    {
        olpc_data->cmd &= ~UPDATE_BLN;
        if (olpc_data->rotatenum < olpc_data->rotatemax)
        {
            olpc_bln_blue_refresh(olpc_data, &wincfg0);
            olpc_bln_cyan_refresh(olpc_data, &wincfg1);
        }
    }

    //refresh screen
    rt_display_win_layers_list(&winhead, &wincfg0);
    rt_display_win_layers_list(&winhead, &wincfg1);
    ret = rt_display_win_layers_set(winhead);
    RT_ASSERT(ret == RT_EOK);

    if (olpc_data->cmd != 0)
    {
        rt_event_send(olpc_data->disp_event, EVENT_BLN_REFRESH);
    }

    //rt_kprintf("ticks = %d\n", rt_tick_get() - ticks);

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * olpc bln touch functions
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)
/**
 * screen touch.
 */
static image_info_t screen_item;
static rt_err_t olpc_bln_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_bln_data *olpc_data = (struct olpc_bln_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_LONG_DOWN:
        rt_event_send(olpc_data->disp_event, EVENT_BLN_EXIT);
        return RT_EOK;

    default:
        break;
    }

    return ret;
}

static rt_err_t olpc_bln_screen_touch_register(void *parameter)
{
    image_info_t *img_info = &screen_item;
    struct olpc_bln_data *olpc_data = (struct olpc_bln_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button touch register */
    {
        screen_item.w = WIN_LAYERS_W;
        screen_item.h = WIN_LAYERS_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_bln_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_bln_screen_touch_unregister(void *parameter)
{
    image_info_t *img_info = &screen_item;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

#endif

/*
 **************************************************************************************************
 *
 * olpc bln demo init & thread
 *
 **************************************************************************************************
 */

/**
 * screen protection timer callback.
 */
static void olpc_bln_timer_callback(void *parameter)
{
    struct olpc_bln_data *olpc_data = (struct olpc_bln_data *)parameter;

    //if (olpc_data->rotatenum * BLN_ROTATE_TICKS < BLN_ROTATE_TIME)
    if (olpc_data->rotatenum < olpc_data->rotatemax)
    {
        olpc_data->cmd |= UPDATE_BLN;
        rt_event_send(olpc_data->disp_event, EVENT_BLN_REFRESH);
    }

    olpc_data->rotatenum += OLPC_ROTATE_PARAM_STEP;
    if (olpc_data->rotatenum * BLN_ROTATE_TICKS >= BLN_BREADTH_TIME * OLPC_ROTATE_PARAM_STEP)
    {
        olpc_data->rotatenum = 0;
    }
}

/**
 * olpc bln dmeo thread.
 */
static void olpc_bln_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_bln_data *olpc_data;

    olpc_data = (struct olpc_bln_data *)rt_malloc(sizeof(struct olpc_bln_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_bln_data));

    olpc_data->cmd = UPDATE_BLN;
    olpc_data->rotatemax = sizeof(bln_rotate_valtab) / sizeof(struct olpc_rotate_parms);
    rt_kprintf("rotate table max num = %d\n", olpc_data->rotatemax);

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_bln_lutset(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_bln_screen_touch_register(olpc_data);
#endif

    olpc_data->disp_event = rt_event_create("display_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->disp_event != RT_NULL);

    ret = olpc_bln_init(olpc_data);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->timer = rt_timer_create("blntmr",
                                       olpc_bln_timer_callback,
                                       (void *)olpc_data,
                                       BLN_ROTATE_TICKS,
                                       RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->timer != RT_NULL);
    rt_timer_start(olpc_data->timer);

    rt_event_send(olpc_data->disp_event, EVENT_BLN_REFRESH);

    while (1)
    {
        ret = rt_event_recv(olpc_data->disp_event,
                            EVENT_BLN_REFRESH | EVENT_BLN_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_BLN_REFRESH)
        {
            ret = olpc_bln_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_BLN_EXIT)
        {
            break;
        }
    }

    /* Thread deinit */
    rt_timer_stop(olpc_data->timer);
    ret = rt_timer_delete(olpc_data->timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->timer = RT_NULL;

#if defined(RT_USING_PISCES_TOUCH)
    olpc_bln_screen_touch_unregister(olpc_data);
    olpc_touch_list_clear();
#endif

    olpc_bln_deinit(olpc_data);

    rt_event_delete(olpc_data->disp_event);
    olpc_data->disp_event = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;

    rt_event_send(olpc_main_event, EVENT_APP_CLOCK);
}

/**
 * olpc bln demo application init.
 */
#if defined(OLPC_DLMODULE_ENABLE)
SECTION(".param") rt_uint16_t dlmodule_thread_priority = 5;
SECTION(".param") rt_uint32_t dlmodule_thread_stacksize = 2048;
int main(int argc, char *argv[])
{
    olpc_bln_thread(RT_NULL);
    return RT_EOK;
}

#else
int olpc_bln_app_init(void)
{
    rt_thread_t rtt_bln;

    rtt_bln = rt_thread_create("olpcbln", olpc_bln_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(rtt_bln != RT_NULL);
    rt_thread_startup(rtt_bln);

    return RT_EOK;
}
//INIT_APP_EXPORT(olpc_bln_app_init);
#endif

#endif
