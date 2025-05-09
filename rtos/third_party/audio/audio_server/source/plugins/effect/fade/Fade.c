/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "Fade.h"

typedef struct
{
    long curWindth;
    long cur_value;
    int  is_l_finished;
    int  is_r_finished;
    int  is_finished;
    long is_in_or_out;
    fade_short l_value;
    fade_short r_value;

} fade_type;

#if 0
_ATTR_AUDIO_DATA_ short x_r[2] = {0, 0};
_ATTR_AUDIO_DATA_ long y_r[2] = {0, 0};

_ATTR_AUDIO_TEXT_
void filter_2_int_r(short *pwBuffer, unsigned short frameLen)
{
    long long y0, y1temp, y2temp;

    short b0;
    short b1;
    short b2;
    short a1;
    short a2;

    short x0;
    short x1;
    short x2;
    long y1;
    long y2;

    short i;

    b0 = 0x2000;
    b1 = 0xffffc000;
    b2 = 0x2000;

    a1 = 0x3fef;
    a2 = 0xffffe011;


    x1 = x_r[0];
    x2 = x_r[1];
    y1 = y_r[0];
    y2 = y_r[1];

    for (i = 0; i < frameLen; i++)
    {
        x0 = *pwBuffer;

        y1temp = (long long)a1 * (long long)y1;
        if (y1temp < 0)
        {
            y1temp = -((-y1temp) >> 13);
            //y0 = y0 >>13;
        }
        else
        {
            y1temp = y1temp >> 13;

        }

        y2temp = (long long)a2 * (long long)y2;
        if (y2temp < 0)
        {
            y2temp = -((-y2temp) >> 13);
            //y0 = y0 >>13;
        }
        else
        {
            y2temp = y2temp >> 13;

        }

        y0 = ((long long)b0 * (long long)x0) + ((long long)b1 * (long long)x1) + ((long long)b2 * (long long)x2) + y1temp + y2temp;
        y2 = y1;
        y1 = y0;
        if (y0 < 0)
        {
            if (y0 <= -268435456)
            {
                y0 = -32768;
            }
            else
            {
                y0 = -((-y0) >> 13);
            }
            //y0 = y0 >>13;
        }
        else
        {
            if (y0 >= 268435454)
            {
                y0 = 32767;
            }
            else
            {
                y0 = y0 >> 13;
            }
        }

        x2 = x1;
        x1 = x0;


        *pwBuffer = y0;
        pwBuffer += 2;
    }

    x_r[0] = x1;
    x_r[1] = x2;
    y_r[0] = y1;
    y_r[1] = y2;

}


_ATTR_AUDIO_TEXT_ short x_l[2] = {0, 0};
_ATTR_AUDIO_TEXT_ long y_l[2] = {0, 0};

_ATTR_AUDIO_TEXT_
void filter_2_int_l(short *pwBuffer, unsigned short frameLen)
{
    long long y0, y1temp, y2temp;

    short b0;
    short b1;
    short b2;
    short a1;
    short a2;

    short x0;
    short x1;
    short x2;
    long y1;
    long y2;

    short i;

    b0 = 0x2000;
    b1 = 0xffffc000;
    b2 = 0x2000;

    a1 = 0x3fef;
    a2 = 0xffffe011;


    x1 = x_l[0];
    x2 = x_l[1];
    y1 = y_l[0];
    y2 = y_l[1];

    for (i = 0; i < frameLen; i++)
    {
        x0 = *pwBuffer;

        y1temp = (long long)a1 * (long long)y1;
        if (y1temp < 0)
        {
            y1temp = -((-y1temp) >> 13);
            //y0 = y0 >>13;
        }
        else
        {
            y1temp = y1temp >> 13;

        }

        y2temp = (long long)a2 * (long long)y2;
        if (y2temp < 0)
        {
            y2temp = -((-y2temp) >> 13);
            //y0 = y0 >>13;
        }
        else
        {
            y2temp = y2temp >> 13;

        }

        y0 = ((long long)b0 * (long long)x0) + ((long long)b1 * (long long)x1) + ((long long)b2 * (long long)x2) + y1temp + y2temp;
        y2 = y1;
        y1 = y0;
        if (y0 < 0)
        {
            if (y0 <= -268435456)
            {
                y0 = -32768;
            }
            else
            {
                y0 = -((-y0) >> 13);
            }
            //y0 = y0 >>13;
        }
        else
        {
            if (y0 >= 268435454)
            {
                y0 = 32767;
            }
            else
            {
                y0 = y0 >> 13;
            }
        }

        x2 = x1;
        x1 = x0;


        *pwBuffer = y0;
        pwBuffer += 2;
    }

    x_l[0] = x1;
    x_l[1] = x2;
    y_l[0] = y1;
    y_l[1] = y2;

}
#else
//extern void filter_2_int_l(short *pwBuffer, unsigned short frameLen);
//extern void filter_2_int_r(short *pwBuffer, unsigned short frameLen);

short x_l[2] = {0, 0};
long long y_l[2] = {0, 0};
short x_r[2] = {0, 0};
long long y_r[2] = {0, 0};

#endif


/*
void DC_filter(short *pwBuffer, unsigned short frameLen)
{
    filter_2_int_l(pwBuffer, frameLen);
    filter_2_int_r(pwBuffer + 1, frameLen);
}
*/
fade_type   Fade_types;
//��ʼ��
//fade: �ṹ��ָ�� begin:��ʼ������� len:���� type: 0-���� 1-����
void FadeInit(long begin, long len, int type)
{

    fade_type *fade;
    fade = &Fade_types;

    Fade_types.is_in_or_out = type;
    //rk_printf("FadeInit Fade_types=%d \n",Fade_types.is_in_or_out);
    if (Fade_types.is_in_or_out == FADE_IN)
    {
        Fade_types.cur_value = 1;
    }
    else
    {
        Fade_types.cur_value = fade_max_coef;
    }

    Fade_types.curWindth = (fade_max_coef / (unsigned short)len);

    fade->is_l_finished = 0;
    fade->is_r_finished = 0;
    fade->is_finished = 0;

}

//����һ��ϵ��(0~65535) �˷������Ӧ������
long FadeDoOnce(void)
{
    //fade_type *fade;

    //fade=&Fade_types;

    if (Fade_types.is_in_or_out == FADE_IN)
    {
        if ((Fade_types.cur_value + Fade_types.curWindth) >= fade_max_coef)
        {
            return fade_max_coef;
        }
        else
        {
            Fade_types.cur_value += Fade_types.curWindth;
            return Fade_types.cur_value;
        }
    }
    else
    {
        if (Fade_types.cur_value <= Fade_types.curWindth)
        {
            return 1;
        }
        else
        {
            Fade_types.cur_value -= Fade_types.curWindth;
            return Fade_types.cur_value;
        }
    }

}

int FadeIsFinished()
{
    fade_type *fade;

    fade = &Fade_types;
    return (fade->is_l_finished & fade->is_r_finished & (fade->is_finished == 2));
}

int FadeGetType()
{
    fade_type *fade;

    fade = &Fade_types;

    return fade->is_in_or_out;
}

int FadeSetFinish()
{
    fade_type *fade;
    fade = &Fade_types;

    fade->is_l_finished = 1;
    fade->is_r_finished = 1;
    return 1;
}

void FadeProcess(fade_short *pwBuffer, unsigned short frameLen)
{

    //int mutel, muter;
    int   effl, effr, i;

    //mutel = 0;
    //muter = 0;

    if (Fade_types.is_l_finished & Fade_types.is_r_finished)
    {
        Fade_types.is_finished++;
    }

    for (i = 0; i < frameLen; i++)
    {
        effr = FadeDoOnce();
        effl = effr;

        if (Fade_types.is_in_or_out == FADE_OUT)
        {
            //rk_printf("!!!FADE_OUT\n");
            if (Fade_types.l_value == 0)
            {
                if (effl == 1)
                {
                    //rk_printf("0::Fade_types.is_l_finished =1");
                    Fade_types.is_l_finished = 1;
                    effl = 0;
                }
            }

            if (Fade_types.r_value == 0)
            {
                if (effr == 1)
                {
                    //rk_printf("0::Fade_types.is_l_finished =1");
                    Fade_types.is_r_finished = 1;
                    effr = 0;
                }
            }

            if (Fade_types.is_l_finished == 1)
            {
                effl = 0;
            }

            if (Fade_types.is_r_finished == 1)
            {
                effr = 0;
            }

        }
        else
        {
            if (effl == fade_max_coef)
            {
                Fade_types.is_l_finished = 1;
                //rk_printf("i::Fade_types.is_l_finished =1");
            }

            if (effr == fade_max_coef)
            {
                Fade_types.is_r_finished = 1;
                //rk_printf("i::Fade_types.is_r_finished =1");
            }
        }
//        if(Fade_types.is_in_or_out)
//           rk_printf("o");
//        else
//           rk_printf("i");

        //rk_printf("!!!pwBuffer[2*%d] = 0x%x +1=0x%x  effl=%d inout=%d\n",i, pwBuffer[2*i], pwBuffer[2*i+1],effl,Fade_types.is_in_or_out);
        pwBuffer[2 * i] = (((fade_long)pwBuffer[2 * i] * effl) >> fade_scale) ; //fade in fade out
        Fade_types.l_value = pwBuffer[2 * i];
        pwBuffer[2 * i + 1] = (((fade_long)pwBuffer[2 * i + 1] * effr) >> fade_scale) ;
        Fade_types.r_value = pwBuffer[2 * i + 1];
        //rk_printf("###pwBuffer[2*%d] = 0x%x +1=0x%x effr =%d inout=%d\n",i, pwBuffer[2*i], pwBuffer[2*i+1],effr,Fade_types.is_in_or_out);

    }
}

