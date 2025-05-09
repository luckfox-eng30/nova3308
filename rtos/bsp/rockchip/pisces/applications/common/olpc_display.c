/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(RT_USING_OLPC_DEMO)
#include <stdio.h>
#include <math.h>

//#include "color_palette.h"
#include "drv_display.h"
#include "drv_heap.h"
#include "image_info.h"
#include "olpc_display.h"
#include "jbig2dec.h"

#define PI 3.1415926535898
#define NULL_CODE           -1      // indicates a NULL prefix
#define CLEAR_CODE          256     // code to flush dictionary and restart decoder
#define FIRST_STRING        257     // code of first dictionary string

/**
 * color palette for RGB332
 */
uint32_t bpp_lut[256] = {0};
RTM_EXPORT(bpp_lut);

/**
 * color palette for RGB332 and BGR233,default format is RGB332.
 */
static  struct rt_display_data *g_disp_data = RT_NULL;

static size_t read_count, write_count;

/**
 * decompress read buffer.
 */
static int rt_decompress_read_buff(image_info_t *img_info)
{
    int value = img_info->data[read_count++];
    if (read_count == img_info->size)
    {
        value = EOF;
    }
    return value;
}

/**
 * decompress write buffer.
 */
static void rt_decompress_write_buff(int value, rt_uint8_t *fb, rt_int32_t fbsize, rt_int32_t xVir, rt_int32_t xoffset, rt_int32_t yoffset, int width)
{
    if (write_count < fbsize)
    {
        xoffset = xoffset + write_count % width;
        yoffset = yoffset + write_count / width;
        *((uint8_t *)((uint32_t)fb + (yoffset)* xVir + xoffset)) = value;
    }

    if (value == EOF)
    {
        return;
    }

    write_count++;
}

/**
 * decompress sub function.
 */
static int rt_display_decompress(image_info_t *img_info, rt_uint8_t *fb, rt_int32_t xVir, rt_int32_t xoffset, rt_int32_t yoffset)
{
    int read_byte, next = FIRST_STRING, prefix = CLEAR_CODE, bits = 0, total_codes;
    unsigned char *terminators, *reverse_buffer;
    unsigned long shifter = 0;
    short *prefixes;

    write_count = 0;
    read_count = 0;

    if ((read_byte = (rt_decompress_read_buff(img_info))) == EOF || (read_byte & 0xfc))  //sanitize first byte
    {
        //rt_kprintf("rt_display_decompress 111\n");
        return RT_ERROR;
    }

    // based on the "maxbits" parameter, compute total codes and allocate dictionary storage
    total_codes = 512 << (read_byte & 0x3);
    reverse_buffer = malloc((total_codes - 256) * sizeof(reverse_buffer[0]));
    prefixes = malloc((total_codes - 256) * sizeof(prefixes[0]));
    terminators = malloc((total_codes - 256) * sizeof(terminators[0]));

    if (!reverse_buffer || !prefixes || !terminators)       // check for mallco() failure
    {
        if (reverse_buffer)
            free(reverse_buffer);
        if (prefixes)
            free(prefixes);
        if (terminators)
            free(terminators);

        //rt_kprintf("rt_display_decompress 222\n");
        return RT_ERROR;
    }

    // This is the main loop where we read input symbols. The values range from 0 to the code value
    // of the "next" string in the dictionary (although the actual "next" code cannot be used yet,
    // and so we reserve that code for the END_CODE). Note that receiving an EOF from the input
    // stream is actually an error because we should have gotten the END_CODE first.
    while (1)
    {
        int code_bits = next < 1024 ? (next < 512 ? 8 : 9) : (next < 2048 ? 10 : 11), code;
        int extras = (1 << (code_bits + 1)) - next - 1;

        do
        {
            if ((read_byte = (rt_decompress_read_buff(img_info))) == EOF)
            {
                free(terminators);
                free(prefixes);
                free(reverse_buffer);

                //rt_kprintf("rt_display_decompress 333\n");
                //return RT_ERROR;
                return RT_EOK;
            }

            shifter |= (long)read_byte << bits;
        }
        while ((bits += 8) < code_bits);

        // first we assume the code will fit in the minimum number of required bits
        code = (int)shifter & ((1 << code_bits) - 1);
        shifter >>= code_bits;
        bits -= code_bits;

        // but if code >= extras, then we need to read another bit to calculate the real code
        // (this is the "adjusted binary" part)
        if (code >= extras)
        {
            if (!bits)
            {
                if ((read_byte = (rt_decompress_read_buff(img_info))) == EOF)
                {
                    free(terminators);
                    free(prefixes);
                    free(reverse_buffer);

                    //rt_kprintf("rt_display_decompress 444\n");
                    //return RT_ERROR;
                    return RT_EOK;
                }

                shifter = (long)read_byte;
                bits = 8;
            }

            code = (code << 1) - extras + (shifter & 1);
            shifter >>= 1;
            bits--;
        }

        if (code == next)                   // sending the maximum code is reserved for the end of the file
            break;
        else if (code == CLEAR_CODE)        // otherwise check for a CLEAR_CODE to start over early
            next = FIRST_STRING;
        else if (prefix == CLEAR_CODE)      // this only happens at the first symbol which is always sent
        {
            rt_decompress_write_buff(code, fb, img_info->w * img_info->h, xVir, xoffset, yoffset, img_info->w);                   // literally and becomes our initial prefix
            next++;
        }
        // Otherwise we have a valid prefix so we step through the string from end to beginning storing the
        // bytes in the "reverse_buffer", and then we send them out in the proper order. One corner-case
        // we have to handle here is that the string might be the same one that is actually being defined
        // now (code == next-1). Also, the first 256 entries of "terminators" and "prefixes" are fixed and
        // not allocated, so that messes things up a bit.
        else
        {
            int cti = (code == next - 1) ? prefix : code;
            unsigned char *rbp = reverse_buffer, c;

            do *rbp++ = cti < 256 ? cti : terminators[cti - 256];      // step backward through string...
            while ((cti = (cti < 256) ? NULL_CODE : prefixes[cti - 256]) != NULL_CODE);

            c = *--rbp;     // the first byte in this string is the terminator for the last string, which is
            // the one that we'll create a new dictionary entry for this time
            do rt_decompress_write_buff(*rbp, fb,  img_info->w * img_info->h, xVir, xoffset, yoffset, img_info->w);                        // send string in corrected order (except for the terminator
            while (rbp-- != reverse_buffer);          // which we don't know yet)

            if (code == next - 1)
                rt_decompress_write_buff(c, fb,  img_info->w * img_info->h, xVir, xoffset, yoffset, img_info->w);

            prefixes[next - 1 - 256] = prefix;     // now update the next dictionary entry with the new string
            terminators[next - 1 - 256] = c;       // (but we're always one behind, so it's not the string just sent)

            if (++next == total_codes)              // check for full dictionary, which forces a reset (and, BTW,
                next = FIRST_STRING;                // means we'll never use the dictionary entry we just wrote)
        }

        prefix = code;      // the code we just received becomes the prefix for the next dictionary string entry
        // (which we'll create once we find out the terminator)
    }

    free(terminators);
    free(prefixes);
    free(reverse_buffer);

    return RT_EOK;
}

/**
 * float angle : rotate angle
 * int w
 * int h :
 * unsigned char *src :
 * unsigned char *dst :
 * int dst_str :
*/
static unsigned char rt_display_SegGradSelect_8bit(int x, int y, int w, int h, int x_res, int y_res, unsigned char *data, unsigned char *src)
{
    unsigned char alpha = 0xff;
    int pos = 0;

    if (x == -1 && y == -1)
    {
        alpha = (x_res * y_res) >> 8;
        pos = (y + 1) * w + (x + 1);
    }
    else if (x == w && y == -1)
    {
        alpha = ((255 - x_res) * y_res) >> 8;
        pos = (y + 1) * w + (x - 1);
    }
    else if (x == -1 && y == h)
    {
        alpha = ((x_res) * (255 - y_res)) >> 8;
        pos = (y - 1) * w + (x + 1);
    }
    else if (x == w && y == h)
    {
        alpha = ((255 - x_res) * (255 - y_res)) >> 8;
        *data = src[(y - 1) * w + (x - 1)];
    }
    else if (x == -1 && y > -1 && y < h)
    {
        alpha = x_res;
        pos = y * w + (x + 1);
    }
    else if (y == -1 && x > -1 && x < w)
    {
        alpha = y_res;
        pos = (y + 1) * w + x;
    }
    else if (x == w && y > -1 && y < h)
    {
        alpha = 255 - x_res;
        pos = y * w + x - 1;
    }
    else if (y == h && x > -1 && x < w)
    {
        alpha = 255 - y_res;
        pos = (y - 1) * w + x;
    }

    *data = src[pos];

    return alpha;
}

static unsigned char rt_display_SegGradSelect_16bit(int x, int y, int w, int h, int x_res, int y_res, unsigned short *data, unsigned short *src)
{
    unsigned char alpha = 0xff;
    int pos = 0;
    if (x == -1 && y == -1)
    {
        alpha = (x_res * y_res) >> 8;
        pos = (y + 1) * w + (x + 1);
    }
    else if (x == w && y == -1)
    {
        alpha = ((255 - x_res) * y_res) >> 8;
        pos = (y + 1) * w + (x - 1);
    }
    else if (x == -1 && y == h)
    {
        alpha = ((x_res) * (255 - y_res)) >> 8;
        pos = (y - 1) * w + (x + 1);
    }
    else if (x == w && y == h)
    {
        alpha = ((255 - x_res) * (255 - y_res)) >> 8;
        *data = src[(y - 1) * w + (x - 1)];
    }
    else if (x == -1 && y > -1 && y < h)
    {
        alpha = x_res;
        pos = y * w + (x + 1);
    }
    else if (y == -1 && x > -1 && x < w)
    {
        alpha = y_res;
        pos = (y + 1) * w + x;
    }
    else if (x == w && y > -1 && y < h)
    {
        alpha = 255 - x_res;
        pos = y * w + x - 1;
    }
    else if (y == h && x > -1 && x < w)
    {
        alpha = 255 - y_res;
        pos = (y - 1) * w + x;
    }

    *data = src[pos];

    return alpha;
}

static unsigned char rt_display_SegGradSelect_32bit(int x, int y, int w, int h, int x_res, int y_res, unsigned int *data, unsigned int *src)
{
    unsigned char alpha = 0xff;
    int pos = 0;
    if (x == -1 && y == -1)
    {
        alpha = (x_res * y_res) >> 8;
        pos = (y + 1) * w + (x + 1);
    }
    else if (x == w && y == -1)
    {
        alpha = ((255 - x_res) * y_res) >> 8;
        pos = (y + 1) * w + (x - 1);
    }
    else if (x == -1 && y == h)
    {
        alpha = ((x_res) * (255 - y_res)) >> 8;
        pos = (y - 1) * w + (x + 1);
    }
    else if (x == w && y == h)
    {
        alpha = ((255 - x_res) * (255 - y_res)) >> 8;
        *data = src[(y - 1) * w + (x - 1)];
    }
    else if (x == -1 && y > -1 && y < h)
    {
        alpha = x_res;
        pos = y * w + (x + 1);
    }
    else if (y == -1 && x > -1 && x < w)
    {
        alpha = y_res;
        pos = (y + 1) * w + x;
    }
    else if (x == w && y > -1 && y < h)
    {
        alpha = 255 - x_res;
        pos = y * w + x - 1;
    }
    else if (y == h && x > -1 && x < w)
    {
        alpha = 255 - y_res;
        pos = (y - 1) * w + x;
    }

    *data = src[pos];

    return alpha;
}

static unsigned char rt_display_SegGradSelect_4bit(int x, int y, int w, int h, int x_res, int y_res, unsigned char *data, unsigned char *src)
{
    unsigned char alpha = 0xff;
    int pos = 0;
    if (x == -1 && y == -1)
    {
        alpha = (x_res * y_res) >> 8;
        pos = (y + 1) * w + (x + 1);
    }
    else if (x == w && y == -1)
    {
        alpha = ((255 - x_res) * y_res) >> 8;
        pos = (y + 1) * w + (x - 1);
    }
    else if (x == -1 && y == h)
    {
        alpha = ((x_res) * (255 - y_res)) >> 8;
        pos = (y - 1) * w + (x + 1);
    }
    else if (x == w && y == h)
    {
        alpha = ((255 - x_res) * (255 - y_res)) >> 8;
        *data = src[(y - 1) * w + (x - 1)];
    }
    else if (x == -1 && y > -1 && y < h)
    {
        alpha = x_res;
        pos = y * w + (x + 1);
    }
    else if (y == -1 && x > -1 && x < w)
    {
        alpha = y_res;
        pos = (y + 1) * w + x;
    }
    else if (x == w && y > -1 && y < h)
    {
        alpha = 255 - x_res;
        pos = y * w + x - 1;
    }
    else if (y == h && x > -1 && x < w)
    {
        alpha = 255 - y_res;
        pos = (y - 1) * w + x;
    }

    if ((pos & 1) == 0)
        *data = src[pos];

    return alpha;
}

/**
 * display rotate.
 */
void rt_display_rotate_32bit(float angle, int w, int h, unsigned int *src, unsigned int *dst, int dst_str, int xcen, int ycen)
{
    int x, y;

    int xlt, xrt, xld, xrd;
    int ylt, yrt, yld, yrd;
    int xmin, xmax, ymin, ymax;

    //rt_kprintf("angle = %d, w = %d, h = %d, src = 0x%08x, dst = 0x%08x, dst_str = %d, xcen = %d, ycen = %d\n",
    //            (int)angle, w, h, src, dst, dst_str, xcen, ycen);

    float cosa = cos((angle * PI) / 180);
    float sina = sin((angle * PI) / 180);

    xlt = cosa * (-xcen) + -sina * (-ycen);
    ylt = sina * (-xcen) +  cosa * (-ycen);
    xrt = cosa * (w - xcen) + -sina * (-ycen);
    yrt = sina * (w - xcen) +  cosa * (-ycen);
    xld = cosa * (-xcen) + -sina * (h - ycen);
    yld = sina * (-xcen) +  cosa * (h - ycen);
    xrd = cosa * (w - xcen) + -sina * (h - ycen);
    yrd = sina * (w - xcen) +  cosa * (h - ycen);

    xmin = MIN(xrd, MIN(xld, MIN(xlt, xrt))) - 1;
    xmax = MAX(xrd, MAX(xld, MAX(xlt, xrt))) + 1;
    ymin = MIN(yrd, MIN(yld, MIN(ylt, yrt))) - 1;
    ymax = MAX(yrd, MAX(yld, MAX(ylt, yrt))) + 1;

    float x_pos, y_pos;
    unsigned char alpha;

    int m = ymin;
    for (int j = ymin; j < ymax; j++)
    {
        int n = xmin;
        for (int i = xmin; i < xmax; i++)
        {
            x_pos = (cosa * i + sina * j) + xcen;
            y_pos = (-sina * i + cosa * j) + ycen;
            x = floor(x_pos);
            y = floor(y_pos);

            if ((x_pos >= 0) && (x_pos < w) && (y_pos >= 0) && (y_pos < h))
            {
                dst[m * dst_str + n] = src[y * w + x];
            }
            else if ((x >= -1) && (y >= -1) && (x <= w) && (y <= h))
            {
                int x_res = (x_pos - x) * (1 << 8);
                int y_res = (y_pos - y) * (1 << 8);
                unsigned int data;

                alpha = rt_display_SegGradSelect_32bit(x, y, w, h, x_res, y_res, &data, src);

                unsigned char /*sa,*/ sr, sg, sb, da, dr, dg, db;

                sr = (dst[m * dst_str + n]) & 0xff;
                sg = (dst[m * dst_str + n] >> 8) & 0xff;
                sb = (dst[m * dst_str + n] >> 16) & 0xff;
                //sa = (dst[m*dst_str + n] >> 24) & 0xff;

                dr = (data) & 0xff;
                dg = (data >> 8) & 0xff;
                db = (data >> 16) & 0xff;
                da = (data >> 24) & 0xff;
                alpha = (alpha * da) >> 8;

                dr = (sr * (255 - alpha) + dr * alpha) >> 8;
                dg = (sg * (255 - alpha) + dg * alpha) >> 8;
                db = (sb * (255 - alpha) + db * alpha) >> 8;

                dst[m * dst_str + n] = dr | (dg << 8) | (db << 16) | (da << 24);
            }
            n++;
        }
        m++;
    }
}

void rt_display_rotate_24bit(float angle, int w, int h, unsigned char *src, unsigned char *dst, int dst_str, int xcen, int ycen)
{
    int x, y;

    int xlt, xrt, xld, xrd;
    int ylt, yrt, yld, yrd;
    int xmin, xmax, ymin, ymax;

    float cosa = cos((angle * PI) / 180);
    float sina = sin((angle * PI) / 180);

    xlt = cosa * (-xcen) + -sina * (-ycen);
    ylt = sina * (-xcen) +  cosa * (-ycen);
    xrt = cosa * (w - xcen) + -sina * (-ycen);
    yrt = sina * (w - xcen) +  cosa * (-ycen);
    xld = cosa * (-xcen) + -sina * (h - ycen);
    yld = sina * (-xcen) +  cosa * (h - ycen);
    xrd = cosa * (w - xcen) + -sina * (h - ycen);
    yrd = sina * (w - xcen) +  cosa * (h - ycen);

    xmin = MIN(xrd, MIN(xld, MIN(xlt, xrt))) - 1;
    xmax = MAX(xrd, MAX(xld, MAX(xlt, xrt))) + 1;
    ymin = MIN(yrd, MIN(yld, MIN(ylt, yrt))) - 1;
    ymax = MAX(yrd, MAX(yld, MAX(ylt, yrt))) + 1;

    float x_pos, y_pos;
    unsigned char alpha;
    //int cur_x_pos;
    //int cur_y_pos;

    int m = ymin;
    for (int j = ymin; j < ymax; j++)
    {
        int n = xmin;
        for (int i = xmin; i < xmax; i++)
        {
            x_pos = (cosa * i + sina * j) + xcen;
            y_pos = (-sina * i + cosa * j) + ycen;
            x = floor(x_pos);
            y = floor(y_pos);

            if (x_pos >= 0 && x_pos < w && y_pos >= 0 && y_pos < h)
            {
                dst[m * dst_str + n] = src[y * w + x];
            }
            else if ((x >= -1) && (y >= -1) && (x <= w) && (y <= h))
            {
                int x_res = (x_pos - x) * (1 << 8);
                int y_res = (y_pos - y) * (1 << 8);
                unsigned char data;

                alpha = rt_display_SegGradSelect_8bit(x, y, w, h, x_res, y_res, &data, src);

                dst[m * dst_str + n] = (dst[m * dst_str + n] * (255 - alpha) + data * alpha) >> 8;
            }
            n++;
        }
        m++;
    }
}
RTM_EXPORT(rt_display_rotate_24bit);

void rt_display_rotate_8bit(float angle, int w, int h, unsigned char *src, unsigned char *dst, int dst_str, int xcen, int ycen)
{
    int x, y;

    int xlt, xrt, xld, xrd;
    int ylt, yrt, yld, yrd;
    int xmin, xmax, ymin, ymax;

    float cosa = cos((angle * PI) / 180);
    float sina = sin((angle * PI) / 180);

    xlt = cosa * (-xcen) + -sina * (-ycen);
    ylt = sina * (-xcen) +  cosa * (-ycen);
    xrt = cosa * (w - xcen) + -sina * (-ycen);
    yrt = sina * (w - xcen) +  cosa * (-ycen);
    xld = cosa * (-xcen) + -sina * (h - ycen);
    yld = sina * (-xcen) +  cosa * (h - ycen);
    xrd = cosa * (w - xcen) + -sina * (h - ycen);
    yrd = sina * (w - xcen) +  cosa * (h - ycen);

    xmin = MIN(xrd, MIN(xld, MIN(xlt, xrt))) - 1;
    xmax = MAX(xrd, MAX(xld, MAX(xlt, xrt))) + 1;
    ymin = MIN(yrd, MIN(yld, MIN(ylt, yrt))) - 1;
    ymax = MAX(yrd, MAX(yld, MAX(ylt, yrt))) + 1;

    float x_pos, y_pos;
    unsigned char alpha;

    int m = ymin;
    for (int j = ymin; j < ymax; j++)
    {
        int n = xmin;
        for (int i = xmin; i < xmax; i++)
        {
            x_pos = (cosa * i + sina * j) + xcen;
            y_pos = (-sina * i + cosa * j) + ycen;
            x = floor(x_pos);
            y = floor(y_pos);

            if (x_pos >= 0 && x_pos < w && y_pos >= 0 && y_pos < h)
            {
                dst[m * dst_str + n] = src[y * w + x];
            }
            else if ((x >= -1) && (y >= -1) && (x <= w) && (y <= h))
            {
                int x_res = (x_pos - x) * (1 << 8);
                int y_res = (y_pos - y) * (1 << 8);
                unsigned char data = 0;

                alpha = rt_display_SegGradSelect_8bit(x, y, w, h, x_res, y_res, &data, src);

                unsigned char sr, sg, sb, dr, dg, db;
                sr = (dst[m * dst_str + n]) & 0x7;
                sg = (dst[m * dst_str + n] >> 3) & 0x7;
                sb = (dst[m * dst_str + n] >> 6) & 0x3;

                dr = (data) & 0x7;
                dg = (data >> 3) & 0x7;
                db = (data >> 6) & 0x3;

                dr = (sr * (255 - alpha) + dr * alpha) >> 8;
                dg = (sg * (255 - alpha) + dg * alpha) >> 8;
                db = (sb * (255 - alpha) + db * alpha) >> 8;

                dst[m * dst_str + n] = dr | (dg << 3) | (db << 6);
            }
            n++;
        }
        m++;
    }
}
RTM_EXPORT(rt_display_rotate_8bit);

void rt_display_rotate_16bit(float angle, int w, int h, unsigned short *src, unsigned short *dst, int dst_str, int xcen, int ycen)
{
    int x, y;

    int xlt, xrt, xld, xrd;
    int ylt, yrt, yld, yrd;
    int xmin, xmax, ymin, ymax;

    float cosa = cos((angle * PI) / 180);
    float sina = sin((angle * PI) / 180);

    xlt = cosa * (-xcen) + -sina * (-ycen);
    ylt = sina * (-xcen) +  cosa * (-ycen);
    xrt = cosa * (w - xcen) + -sina * (-ycen);
    yrt = sina * (w - xcen) +  cosa * (-ycen);
    xld = cosa * (-xcen) + -sina * (h - ycen);
    yld = sina * (-xcen) +  cosa * (h - ycen);
    xrd = cosa * (w - xcen) + -sina * (h - ycen);
    yrd = sina * (w - xcen) +  cosa * (h - ycen);

    xmin = MIN(xrd, MIN(xld, MIN(xlt, xrt))) - 1;
    xmax = MAX(xrd, MAX(xld, MAX(xlt, xrt))) + 1;
    ymin = MIN(yrd, MIN(yld, MIN(ylt, yrt))) - 1;
    ymax = MAX(yrd, MAX(yld, MAX(ylt, yrt))) + 1;

    float x_pos, y_pos;
    unsigned char alpha;

    int m = ymin;
    for (int j = ymin; j < ymax; j++)
    {
        int n = xmin;
        for (int i = xmin; i < xmax; i++)
        {
            x_pos = (cosa * i + sina * j) + xcen;
            y_pos = (-sina * i + cosa * j) + ycen;
            x = floor(x_pos);
            y = floor(y_pos);

            if (x_pos >= 0 && x_pos < w && y_pos >= 0 && y_pos < h)
            {
                dst[m * dst_str + n] = src[y * w + x];
            }
            else if ((x >= -1) && (y >= -1) && (x <= w) && (y <= h))
            {
                int x_res = (x_pos - x) * (1 << 8);
                int y_res = (y_pos - y) * (1 << 8);
                unsigned short data = 0;

                alpha = rt_display_SegGradSelect_16bit(x, y, w, h, x_res, y_res, &data, src);

                unsigned char sr, sg, sb, dr, dg, db;
                sr = (dst[m * dst_str + n]) & 0x1f;
                sg = (dst[m * dst_str + n] >> 5) & 0x3f;
                sb = (dst[m * dst_str + n] >> 11) & 0x1f;

                dr = (data) & 0x1f;
                dg = (data >> 5) & 0x3f;
                db = (data >> 11) & 0x1f;

                dr = (sr * (255 - alpha) + dr * alpha) >> 8;
                dg = (sg * (255 - alpha) + dg * alpha) >> 8;
                db = (sb * (255 - alpha) + db * alpha) >> 8;

                dst[m * dst_str + n] = dr | (dg << 5) | (db << 11);
            }
            n++;
        }
        m++;
    }
}
RTM_EXPORT(rt_display_rotate_16bit);

void rt_display_rotate_4bit(float angle, int w, int h, unsigned char *src, unsigned char *dst, int dst_str, int xcen, int ycen)
{
    int x, y;

    int xlt, xrt, xld, xrd;
    int ylt, yrt, yld, yrd;
    int xmin, xmax, ymin, ymax;

    float cosa = cos((angle * PI) / 180);
    float sina = sin((angle * PI) / 180);

    xlt = cosa * (-xcen) + -sina * (-ycen);
    ylt = sina * (-xcen) +  cosa * (-ycen);
    xrt = cosa * (w - xcen) + -sina * (-ycen);
    yrt = sina * (w - xcen) +  cosa * (-ycen);
    xld = cosa * (-xcen) + -sina * (h - ycen);
    yld = sina * (-xcen) +  cosa * (h - ycen);
    xrd = cosa * (w - xcen) + -sina * (h - ycen);
    yrd = sina * (w - xcen) +  cosa * (h - ycen);

    xmin = MIN(xrd, MIN(xld, MIN(xlt, xrt))) - 1;
    xmax = MAX(xrd, MAX(xld, MAX(xlt, xrt))) + 1;
    ymin = MIN(yrd, MIN(yld, MIN(ylt, yrt))) - 1;
    ymax = MAX(yrd, MAX(yld, MAX(ylt, yrt))) + 1;

    float x_pos, y_pos;
    unsigned char alpha;

    int m = ymin;
    for (int j = ymin; j < ymax; j++)
    {
        int n = xmin;
        for (int i = xmin; i < xmax; i++)
        {
            x_pos = (cosa * i + sina * j) + xcen;
            y_pos = (-sina * i + cosa * j) + ycen;
            x = floor(x_pos);
            y = floor(y_pos);

            int dst_pos = ((m * dst_str) + n) >> 1;
            int src_pos = ((y * w >> 1) + x) >> 1;

            if (x_pos >= 0 && x_pos < w && y_pos >= 0 && y_pos < h)
            {
                if ((x & 1) == 0)
                {
                    dst[dst_pos] = (dst[dst_pos] & 0xf0) | ((src[src_pos] & 0xf));
                }
                else
                {
                    dst[dst_pos] = (dst[dst_pos] & 0x0f) | ((src[src_pos] & 0xf) << 4);
                }
            }
            else if ((x >= -1) && (y >= -1) && (x <= w) && (y <= h))
            {
                int x_res = (x_pos - x) * (1 << 8);
                int y_res = (y_pos - y) * (1 << 8);
                unsigned char data = 0;

                alpha = rt_display_SegGradSelect_4bit(x, y, w, h, x_res, y_res, &data, src);

                unsigned char sr, sg, sb, dr, dg, db;
                sr = (dst[m * dst_str + n]) & 0x7;
                sg = (dst[m * dst_str + n] >> 3) & 0x7;
                sb = (dst[m * dst_str + n] >> 6) & 0x3;

                dr = (data) & 0x7;
                dg = (data >> 3) & 0x7;
                db = (data >> 6) & 0x3;

                dr = (sr * (255 - alpha) + dr * alpha) >> 8;
                dg = (sg * (255 - alpha) + dg * alpha) >> 8;
                db = (sb * (255 - alpha) + db * alpha) >> 8;

                dst[m * dst_str + n] = dr | (dg << 3) | (db << 6);
            }
            n++;
        }
        m++;
    }
}
RTM_EXPORT(rt_display_rotate_4bit);

/**
 * color palette for RGB332 and BGR233,default format is RGB332.
 */
void rt_display_update_lut(int format)
{
    int i = 0;
    int r2, r1, r0, g2, g1, g0, b1, b0;
    int R, G, B;
    float f;

    for (i = 0; i < 256; i++)
    {
        if (format == FORMAT_RGB_332)
        {
            r2 = (i & 0x80) >> 7;
            r1 = (i & 0x40) >> 6;
            r0 = (i & 0x20) >> 5;
            g2 = (i & 0x10) >> 4;
            g1 = (i & 0x8) >> 3;
            g0 = (i & 0x4) >> 2;
            b1 = (i & 0x2) >> 1;
            b0 = (i & 0x1) >> 0;
            R = (r2 << 7) | (r1 << 6) | (r0 << 5) | (r2 << 4) | (r1 << 3) | (r0 << 2) | (r2 << 1) | r1;
            G = (g2 << 7) | (g1 << 6) | (g0 << 5) | (g2 << 4) | (g1 << 3) | (g0 << 2) | (g2 << 1) | g1;
            B = (b1 << 7) | (b0 << 6) | (b1 << 5) | (b0 << 4) | (b1 << 3) | (b0 << 2) | (b1 << 1) | b0;
        }
        else if (format == FORMAT_BGR_233)
        {
            b1 = (i & 0x80) >> 7;
            b0 = (i & 0x40) >> 6;
            g2 = (i & 0x20) >> 5;
            g1 = (i & 0x10) >> 4;
            g0 = (i & 0x8) >> 3;
            r2 = (i & 0x4) >> 2;
            r1 = (i & 0x2) >> 1;
            r0 = (i & 0x1) >> 0;
            R = (r2 << 7) | (r1 << 6) | (r0 << 5) | (r2 << 4) | (r1 << 3) | (r0 << 2) | (r2 << 1) | r1;
            G = (g2 << 7) | (g1 << 6) | (g0 << 5) | (g2 << 4) | (g1 << 3) | (g0 << 2) | (g2 << 1) | g1;
            B = (b1 << 7) | (b0 << 6) | (b1 << 5) | (b0 << 4) | (b1 << 3) | (b0 << 2) | (b1 << 1) | b0;
        }
        else
        {
            f = (i + 0.5F) / 256;
            R = (unsigned char)((float)pow(f, 1 / GAMMA_RED) * 255 - 0.5F);
            G = (unsigned char)((float)pow(f, 1 / GAMMA_GREEN) * 255 - 0.5F);
            B = (unsigned char)((float)pow(f, 1 / GAMMA_BLUE) * 255 - 0.5F);
        }
        bpp_lut[i] = (R << 16) | (G << 8) | B;

        /* if (i % 4 == 0)
            printf("\n");
        printf("0x%08x, ", bpp_lut[i]); */
    }
}
RTM_EXPORT(rt_display_update_lut);

/**
 * Check is if two display region overlapped
 */
rt_err_t olpc_display_overlay_check(rt_uint16_t y1, rt_uint16_t y2, rt_uint16_t yy1, rt_uint16_t yy2)
{
    if ((y2 < yy1) || (y1 > yy2))
    {
        // no overlay
        return RT_EOK;
    }

    // overlay
    return RT_ERROR;
}
RTM_EXPORT(olpc_display_overlay_check);

/**
 * fill image data to fb buffer
 */
void rt_display_img_fill(image_info_t *img_info, rt_uint8_t *fb, rt_int32_t xVir, rt_int32_t xoffset, rt_int32_t yoffset)
{
    rt_err_t ret = RT_EOK;
    rt_int32_t x, y, i;
    rt_uint8_t bitval;

    if (img_info->type == IMG_TYPE_COMPRESS)
    {
        if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_RGB332)
        {
            ret = rt_display_decompress(img_info, fb, xVir, xoffset, yoffset);
            RT_ASSERT(ret == RT_EOK);
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_RGB565)
        {
            image_info_t info;
            rt_memcpy(&info, img_info, sizeof(image_info_t));
            info.x  *= 2;
            info.w  *= 2;
            xVir    *= 2;
            xoffset *= 2;
            ret = rt_display_decompress(&info, fb, xVir, xoffset, yoffset);
            RT_ASSERT(ret == RT_EOK);
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_ARGB888)
        {
            image_info_t info;
            rt_memcpy(&info, img_info, sizeof(image_info_t));
            info.x  *= 4;
            info.w  *= 4;
            xVir    *= 4;
            xoffset *= 4;
            ret = rt_display_decompress(&info, fb, xVir, xoffset, yoffset);
            RT_ASSERT(ret == RT_EOK);
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_GRAY16)
        {
            image_info_t info;
            rt_memcpy(&info, img_info, sizeof(image_info_t));
            info.x  /= 2;
            info.w  /= 2;
            xVir    /= 2;
            xoffset /= 2;
            ret = rt_display_decompress(&info, fb, xVir, xoffset, yoffset);
            RT_ASSERT(ret == RT_EOK);
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_GRAY4)
        {
            image_info_t info;
            rt_memcpy(&info, img_info, sizeof(image_info_t));
            info.x  /= 4;
            info.w  /= 4;
            xVir    /= 4;
            xoffset /= 4;
            ret = rt_display_decompress(&info, fb, xVir, xoffset, yoffset);
            RT_ASSERT(ret == RT_EOK);
        }
        else //if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_GRAY1)
        {
            ret = jbig2_decompression(img_info, fb, xVir, xoffset, yoffset);
            RT_ASSERT(ret == 0);
        }
    }
    else //if (img_info->type == IMG_TYPE_RAW)
    {
        if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_RGB332)
        {
            for (i = 0, y = yoffset; y < yoffset + img_info->h; y++)
            {
                for (x = xoffset; x < xoffset + img_info->w; x++)
                {
                    fb[(y * xVir) + x] = img_info->data[i++];
                }
            }
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_RGB565)
        {
            for (i = 0, y = yoffset; y < yoffset + img_info->h; y++)
            {
                for (x = xoffset * 2; x < xoffset * 2 + img_info->w * 2; x++)
                {
                    fb[(y * xVir * 2) + x] = img_info->data[i++];
                }
            }
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_ARGB888)
        {
            for (i = 0, y = yoffset; y < yoffset + img_info->h; y++)
            {
                for (x = xoffset * 4; x < xoffset * 4 + img_info->w * 4; x++)
                {
                    fb[(y * xVir * 4) + x] = img_info->data[i++];
                }
            }
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_GRAY1)
        {
            RT_ASSERT((xVir % 8) == 0);

            rt_uint8_t colorkey = (rt_uint8_t)(img_info->colorkey & 0xff);

            if (((xoffset % 8) == 0) && ((img_info->colorkey & COLOR_KEY_EN) == 0))
            {
                for (y = yoffset; y < yoffset + img_info->h; y++)
                {
                    i = (y - yoffset) * ((img_info->w + 7) / 8);
                    for (x = xoffset / 8; x < (xoffset + img_info->w) / 8; x++)
                    {
                        fb[y * (xVir / 8) + x] = img_info->data[i++];
                    }

                    if (((xoffset + img_info->w) % 8) != 0)
                    {
                        rt_uint8_t maskval = 0xff >> (img_info->w % 8);
                        fb[y * (xVir / 8) + x] &= maskval;
                        fb[y * (xVir / 8) + x] |= (img_info->data[i++] & (~maskval));
                    }
                }
            }
            else
            {
                for (y = yoffset; y < yoffset + img_info->h; y++)
                {
                    i = (y - yoffset) * ((img_info->w + 7) / 8) * 8;
                    for (x = xoffset; x < xoffset + img_info->w; x++)
                    {
                        bitval = (img_info->data[i / 8] << (i % 8)) & 0x80;

                        i++;
                        if (img_info->colorkey & COLOR_KEY_EN)
                        {
                            if (((colorkey != 0) && (bitval != 0)) ||
                                    ((colorkey == 0) && (bitval == 0)))
                            {
                                continue;
                            }
                        }

                        fb[y * (xVir / 8) + x / 8] &= ~(0x80 >> (x % 8));
                        fb[y * (xVir / 8) + x / 8] |= bitval >> (x % 8);
                    }
                }
            }
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_GRAY4)
        {
            RT_ASSERT((xVir % 4) == 0);

            if ((xoffset % 4) == 0)
            {
                for (y = yoffset; y < yoffset + img_info->h; y++)
                {
                    i = (y - yoffset) * ((img_info->w + 3) / 4);
                    for (x = xoffset / 4; x < (xoffset + img_info->w) / 4; x++)
                    {
                        fb[y * (xVir / 4) + x] = img_info->data[i++];
                    }

                    if (((xoffset + img_info->w) % 4) != 0)
                    {
                        rt_uint8_t maskval = 0xff >> (img_info->w % 4);
                        fb[y * (xVir / 4) + x] &= maskval;
                        fb[y * (xVir / 4) + x] |= (img_info->data[i++] & (~maskval));
                    }
                }
            }
            else
            {
                for (y = yoffset; y < yoffset + img_info->h; y++)
                {
                    i = (y - yoffset) * ((img_info->w + 3) / 4) * 4;
                    for (x = xoffset; x < xoffset + img_info->w; x++)
                    {
                        bitval = (img_info->data[i / 4] << (i % 4)) & 0xc0;
                        i++;

                        fb[y * (xVir / 4) + x / 4] &= ~(0xc0 >> (x % 4));
                        fb[y * (xVir / 4) + x / 4] |= bitval >> (x % 4);
                    }
                }
            }
        }
        else if (img_info->pixel == RTGRAPHIC_PIXEL_FORMAT_GRAY16)
        {
            RT_ASSERT((xVir % 2) == 0);

            if ((xoffset % 2) == 0)
            {
                for (y = yoffset; y < yoffset + img_info->h; y++)
                {
                    i = (y - yoffset) * ((img_info->w + 1) / 2);
                    for (x = xoffset / 2; x < (xoffset + img_info->w) / 2; x++)
                    {
                        fb[y * (xVir / 2) + x] = img_info->data[i++];
                    }

                    if (((xoffset + img_info->w) % 2) != 0)
                    {
                        rt_uint8_t maskval = 0xff >> (img_info->w % 2);
                        fb[y * (xVir / 2) + x] &= maskval;
                        fb[y * (xVir / 2) + x] |= (img_info->data[i++] & (~maskval));
                    }
                }
            }
            else
            {
                for (y = yoffset; y < yoffset + img_info->h; y++)
                {
                    i = (y - yoffset) * ((img_info->w + 1) / 2) * 2;
                    for (x = xoffset; x < xoffset + img_info->w; x++)
                    {
                        bitval = (img_info->data[i / 2] << (i % 2)) & 0xf0;
                        i++;

                        fb[y * (xVir / 2) + x / 2] &= ~(0xf0 >> (x % 2));
                        fb[y * (xVir / 2) + x / 2] |= bitval >> (x % 2);
                    }
                }
            }
        }
    }
}
RTM_EXPORT(rt_display_img_fill);

/**
 * list win layers.
 */
rt_err_t rt_display_win_layers_list(struct rt_display_config **head, struct rt_display_config *tail)
{
    RT_ASSERT(tail != RT_NULL);

    if (*head == RT_NULL)
    {
        *head = tail;
    }
    else
    {
        struct rt_display_config *tmp = *head;
        while (tmp->next != RT_NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = tail;
        tail->next = RT_NULL;
    }

    return RT_EOK;
}
RTM_EXPORT(rt_display_win_layers_list);

/**
 * Configuration win layers.
 */
rt_err_t rt_display_win_layers_set(struct rt_display_config *wincfg)
{
    rt_err_t ret = RT_EOK;
    struct CRTC_WIN_STATE win_config;
    struct VOP_POST_SCALE_INFO post_scale;
    struct rt_display_data *disp_data = g_disp_data;
    struct rt_device_graphic_info info;
    rt_device_t device = disp_data->device;
    struct rt_display_config *cfg;

    RT_ASSERT(wincfg != RT_NULL);

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RK_DISPLAY_CTRL_ENABLE, NULL);
    RT_ASSERT(ret == RT_EOK);

    /* post scale set */
    cfg = wincfg;
    if (1)
    {
        rt_memset(&post_scale, 0, sizeof(struct VOP_POST_SCALE_INFO));
        post_scale.dstX = 0;
        post_scale.dstY = 0xffff;
        post_scale.srcW = disp_data->xres;
        post_scale.srcH = 0;

        rt_uint16_t dstY2 = 0;

        while (cfg != RT_NULL)
        {
            post_scale.dstY = MIN(post_scale.dstY, cfg->y);
            post_scale.dstY = MIN(post_scale.dstY, cfg->ylast);
            dstY2 = MAX(dstY2, cfg->y + cfg->h);
            dstY2 = MAX(dstY2, cfg->ylast + cfg->h);

            cfg = cfg->next;
        }

        post_scale.srcH = dstY2 - post_scale.dstY;
        post_scale.dstW = post_scale.srcW * WSCALE;
        post_scale.dstH = post_scale.srcH * HSCALE;

        //rt_kprintf("dstX = %d, dstY = %d, dstW = %d, dstH =%d, srcW = %d, srcH =%d\n",
        //            post_scale.dstX, post_scale.dstY, post_scale.dstW, post_scale.dstH,
        //            post_scale.srcW,  post_scale.srcH);

        ret = rt_device_control(device,
                                RK_DISPLAY_CTRL_SET_SCALE, &post_scale);
        RT_ASSERT(ret == RT_EOK);
    }

    /* win0 set */
    cfg = wincfg;
    while (cfg != RT_NULL)
    {
        rt_memset(&win_config, 0, sizeof(struct CRTC_WIN_STATE));
        win_config.winId = cfg->winId;
        win_config.winEn = 1;
        win_config.winUpdate = 1;

        win_config.zpos = cfg->zpos;

        win_config.yrgbAddr = (uint32_t)cfg->fb;
        win_config.cbcrAddr = (uint32_t)cfg->fb;
        win_config.yrgbLength = cfg->fblen;
        win_config.cbcrLength = cfg->fblen;
        win_config.colorKey   = cfg->colorkey;

        win_config.xVir  = cfg->w;
        win_config.srcX  = cfg->x;
        win_config.srcY  = cfg->y;
        win_config.srcW  = cfg->w;
        win_config.srcH  = cfg->h;

        win_config.xLoopOffset = 0;
        win_config.yLoopOffset = 0;

        win_config.alphaEn = cfg->alphaEn;
        win_config.alphaMode = cfg->alphaMode;
        win_config.alphaPreMul = cfg->alphaPreMul;
        win_config.globalAlphaValue = cfg->globalAlphaValue;

        win_config.lut    = disp_data->lut[cfg->winId].lut;
        win_config.format = disp_data->lut[cfg->winId].format;
#if 0
        rt_kprintf("winId = %d, srcX = %d, srcY = %d, srcW = %d, srcH =%d, lut = %d, format =%d\n",
                   win_config.winId, win_config.srcX, win_config.srcY, win_config.srcW, win_config.srcH,
                   win_config.lut,  win_config.format);
        rt_kprintf("alphaEn = %d, alphaMode = %d, alphaPreMul = %d, globalAlphaValue = %d\n",
                   win_config.alphaEn, win_config.alphaMode, win_config.alphaPreMul, win_config.globalAlphaValue);
#endif
        ret = rt_device_control(device, RK_DISPLAY_CTRL_SET_PLANE, &win_config);
        RT_ASSERT(ret == RT_EOK);

        cfg = cfg->next;
    }

    ret = rt_device_control(device, RK_DISPLAY_CTRL_COMMIT, NULL);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_display_sync_hook(device);
    RT_ASSERT(ret == RT_EOK);

    // close win
    cfg = wincfg;
    while (cfg != RT_NULL)
    {
        rt_memset(&win_config, 0, sizeof(struct CRTC_WIN_STATE));
        win_config.winId = cfg->winId;
        win_config.winEn = 0;
        win_config.winUpdate = 1;
        ret = rt_device_control(device, RK_DISPLAY_CTRL_SET_PLANE, &win_config);
        RT_ASSERT(ret == RT_EOK);

        cfg = cfg->next;
    }

    ret = rt_device_control(device, RK_DISPLAY_CTRL_DISABLE, NULL);
    RT_ASSERT(ret == RT_EOK);

    return ret;
}
RTM_EXPORT(rt_display_win_layers_set);

/**
 * backlight set.
 */
rt_err_t rt_display_win_backlight_set(rt_uint16_t val)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_data *disp_data = g_disp_data;
    rt_device_t device = disp_data->device;
    rt_uint16_t blval  = val;

    ret = rt_device_control(device, RK_DISPLAY_CTRL_ENABLE, NULL);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RK_DISPLAY_CTRL_UPDATE_BL, &blval);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RK_DISPLAY_CTRL_DISABLE, NULL);
    RT_ASSERT(ret == RT_EOK);

    return ret;
}
RTM_EXPORT(rt_display_win_backlight_set);

/**
 * Clear screen for display initial.
 */
int rt_display_screen_clear(rt_device_t device)
{
    rt_err_t ret = RT_EOK;
    struct CRTC_WIN_STATE win_config;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    rt_memset(&win_config, 0, sizeof(struct CRTC_WIN_STATE));
    win_config.winEn = 0;
    win_config.winId = 0;
    win_config.winUpdate = 1;

    win_config.srcW = info.width;
    win_config.srcH = info.height;
    win_config.crtcX = 0;
    win_config.crtcY = 0;
    win_config.crtcW = info.width;
    win_config.crtcH = info.height;

    ret = rt_device_control(device, RK_DISPLAY_CTRL_SET_PLANE, &win_config);
    RT_ASSERT(ret == RT_EOK);

    rt_thread_delay(200);

    return ret;
}
RTM_EXPORT(rt_display_screen_clear);

/**
 * Display screen scroll API.
 */
rt_err_t rt_display_screen_scroll(rt_device_t device, uint8_t winId, uint32_t mode,
                                  uint16_t srcW, uint16_t srcH,
                                  int16_t xoffset, int16_t yoffset)
{
    int ret;
    struct display_state *state = (struct display_state *)device->user_data;
    struct CRTC_WIN_STATE win_config, *win_state = &(state->crtc_state.win_state[winId]);
    struct VOP_POST_SCALE_INFO post_scale;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    memcpy(&win_config, win_state, sizeof(struct CRTC_WIN_STATE));

    win_config.winEn = 1;
    win_config.winId = winId;
    win_config.winUpdate = 1;

    win_config.xLoopOffset += xoffset;
    if (win_config.xLoopOffset >= srcW)
    {
        win_config.xLoopOffset = 0;
    }

    win_config.yLoopOffset += yoffset;
    if (win_config.yLoopOffset >= srcH)
    {
        win_config.yLoopOffset = 0;
    }

    post_scale.srcW = g_disp_data->xres;
    post_scale.srcH = win_config.srcH;
    post_scale.dstX = 0;
    post_scale.dstY = win_config.srcY;
    post_scale.dstW = post_scale.srcW * WSCALE;
    post_scale.dstH = post_scale.srcH * HSCALE;

    ret = rt_device_control(device, RK_DISPLAY_CTRL_SET_SCALE, &post_scale);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RK_DISPLAY_CTRL_SET_PLANE, &win_config);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RK_DISPLAY_CTRL_COMMIT, NULL);
    RT_ASSERT(ret == RT_EOK);

    return RT_EOK;
}
RTM_EXPORT(rt_display_screen_scroll);

/**
 * Display driver sync hook, wait for drv_display finish.
 */
rt_err_t rt_display_sync_hook(rt_device_t device)
{
    int ret, i;
    struct display_sync display_sync_data;
    display_sync_data.cmd = DISPLAY_SYNC;

    for (i = 0; i < 100; i++)
    {
        rt_thread_mdelay(1);
        ret = rt_device_control(device, RK_DISPLAY_CTRL_DISPLAY_SYNC, &display_sync_data);
        if (ret == RT_EOK)
            break;
    }
    if (i == 100)
        rt_kprintf("rt_display_sync_hook time out\n");

    return RT_EOK;
}
RTM_EXPORT(rt_display_sync_hook);

/**
 * Get MAX value of backlight.
 */
rt_uint16_t rt_display_get_bl_max(rt_device_t device)
{
    struct display_state *state = (struct display_state *)device->user_data;

    return state->panel_state.max_brightness;
}
RTM_EXPORT(rt_display_get_bl_max);

/**
 * Display lut set.
 */
static rt_uint8_t fmt2bps[RTGRAPHIC_PIXEL_FORMAT_VYUY422_4 + 1] =
{
    1,  //RTGRAPHIC_PIXEL_FORMAT_MONO = 0,
    1,  //RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    2,  //RTGRAPHIC_PIXEL_FORMAT_GRAY4,
    4,  //RTGRAPHIC_PIXEL_FORMAT_GRAY16,
    8,  //RTGRAPHIC_PIXEL_FORMAT_GRAY256,
    8,  //RTGRAPHIC_PIXEL_FORMAT_RGB332,
    16, //RTGRAPHIC_PIXEL_FORMAT_RGB444,
    16, //RTGRAPHIC_PIXEL_FORMAT_RGB565,
    16, //RTGRAPHIC_PIXEL_FORMAT_BGR565 = RTGRAPHIC_PIXEL_FORMAT_RGB565P,
    24, //RTGRAPHIC_PIXEL_FORMAT_RGB666,
    24, //RTGRAPHIC_PIXEL_FORMAT_RGB888,
    32, //RTGRAPHIC_PIXEL_FORMAT_ARGB888,
    32, //RTGRAPHIC_PIXEL_FORMAT_ABGR888,
    24, //RTGRAPHIC_PIXEL_FORMAT_ARGB565,
    32,  //RTGRAPHIC_PIXEL_FORMAT_ALPHA,
    32, //RTGRAPHIC_PIXEL_FORMAT_YUV420,
    32, //RTGRAPHIC_PIXEL_FORMAT_YUV422,
    32, //RTGRAPHIC_PIXEL_FORMAT_YUV444,
    32, //RTGRAPHIC_PIXEL_FORMAT_YVYU422,
    32, //RTGRAPHIC_PIXEL_FORMAT_VYUY422,
    32, //RTGRAPHIC_PIXEL_FORMAT_YUV420_4,
    32, //RTGRAPHIC_PIXEL_FORMAT_YUV422_4,
    32, //RTGRAPHIC_PIXEL_FORMAT_YUV444_4,
    32, //RTGRAPHIC_PIXEL_FORMAT_YVYU422_4,
    32, //RTGRAPHIC_PIXEL_FORMAT_VYUY422_4,
};
rt_err_t rt_display_win_clear(rt_uint8_t winid, rt_uint8_t fmt,
                              rt_uint16_t y, rt_uint16_t h, rt_uint8_t data)
{
    rt_err_t ret;
    struct rt_display_config wincfg;

    RT_ASSERT((y % 2) == 0);
    RT_ASSERT((h % 2) == 0);

    rt_memset(&wincfg, 0, sizeof(struct rt_display_config));
    wincfg.winId = winid;
    wincfg.x     = 0;
    wincfg.y     = y;
    wincfg.w     = MAX(32 / fmt2bps[fmt], 1);
    wincfg.h     = h;
    wincfg.fblen = wincfg.w * wincfg.h * fmt2bps[fmt] / 8;
    wincfg.fb    = (rt_uint8_t *)rt_malloc_large(wincfg.fblen);
    rt_memset((void *)wincfg.fb, data, wincfg.fblen);

    ret = rt_display_win_layers_set(&wincfg);
    RT_ASSERT(ret == RT_EOK);

    rt_free_large(wincfg.fb);

    return RT_EOK;
}
RTM_EXPORT(rt_display_win_clear);

/**
 * Display lut set.
 */
rt_err_t rt_display_lutset(struct rt_display_lut *lutA,
                           struct rt_display_lut *lutB,
                           struct rt_display_lut *lutC)
{
    rt_err_t ret;
    rt_display_data_t disp_data = g_disp_data;
    rt_device_t device = disp_data->device;
    struct crtc_lut_state lut_state;

    // close bpp mode and refresh frame
    if ((lutA != RT_NULL) && (lutA->size != 0))
    {
        disp_data->lut[lutA->winId].lut    = RT_NULL;
        disp_data->lut[lutA->winId].format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        rt_display_win_clear(lutA->winId, RTGRAPHIC_PIXEL_FORMAT_RGB565, 0, 2, 0);
    }

    if ((lutB != RT_NULL) && (lutB->size != 0))
    {
        disp_data->lut[lutB->winId].lut    = RT_NULL;
        disp_data->lut[lutB->winId].format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        rt_display_win_clear(lutB->winId, RTGRAPHIC_PIXEL_FORMAT_RGB565, 0, 2, 0);
    }

    if ((lutC != RT_NULL) && (lutC->size != 0))
    {
        disp_data->lut[lutC->winId].lut    = RT_NULL;
        disp_data->lut[lutC->winId].format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        rt_display_win_clear(lutC->winId, RTGRAPHIC_PIXEL_FORMAT_RGB565, 0, 2, 0);
    }

    // set new lut......
    ret = rt_device_control(device, RTGRAPHIC_CTRL_POWERON, NULL);
    RT_ASSERT(ret == RT_EOK);

    memset(&lut_state, 0, sizeof(struct crtc_lut_state));
    if (lutA != RT_NULL)
    {
        disp_data->lut[lutA->winId].lut    = lutA->lut;
        disp_data->lut[lutA->winId].format = lutA->format;

        if (lutA->size)
        {
            lut_state.win_id = lutA->winId;
            lut_state.lut = lutA->lut;
            lut_state.lut_size = lutA->size;
            ret = rt_device_control(device, RK_DISPLAY_CTRL_LOAD_LUT, &lut_state);
            RT_ASSERT(ret == RT_EOK);
        }
    }

    if (lutB != RT_NULL)
    {
        disp_data->lut[lutB->winId].lut    = lutB->lut;
        disp_data->lut[lutB->winId].format = lutB->format;

        if (lutB->size)
        {
            lut_state.win_id = lutB->winId;
            lut_state.lut = lutB->lut;
            lut_state.lut_size = lutB->size;
            ret = rt_device_control(device, RK_DISPLAY_CTRL_LOAD_LUT, &lut_state);
            RT_ASSERT(ret == RT_EOK);
        }
    }

    if (lutC != RT_NULL)
    {
        disp_data->lut[lutC->winId].lut    = lutC->lut;
        disp_data->lut[lutC->winId].format = lutC->format;

        if (lutC->size)
        {
            lut_state.win_id = lutC->winId;
            lut_state.lut = lutC->lut;
            lut_state.lut_size = lutC->size;
            ret = rt_device_control(device, RK_DISPLAY_CTRL_LOAD_LUT, &lut_state);
            RT_ASSERT(ret == RT_EOK);
        }
    }

    ret = rt_device_control(device, RTGRAPHIC_CTRL_POWEROFF, NULL);
    RT_ASSERT(ret == RT_EOK);

    // enable new lut and refresh frame
    if ((lutA != RT_NULL) && (lutA->size != 0))
    {
        rt_display_win_clear(lutA->winId, lutA->format, 0, 2, 0);
    }

    if ((lutB != RT_NULL) && (lutB->size != 0))
    {
        rt_display_win_clear(lutB->winId, lutB->format, 0, 2, 0);
    }

    if ((lutC != RT_NULL) && (lutC->size != 0))
    {
        rt_display_win_clear(lutC->winId, lutC->format, 0, 2, 0);
    }

    return RT_EOK;
}
RTM_EXPORT(rt_display_lutset);

/**
 * Display application initial, initial screen and win layers.
 */
rt_display_data_t rt_display_init(struct rt_display_lut *lutA,
                                  struct rt_display_lut *lutB,
                                  struct rt_display_lut *lutC)
{
    rt_err_t ret = RT_EOK;
    rt_device_t device;
    rt_display_data_t disp_data;
    struct rt_device_graphic_info info;
    struct crtc_lut_state lut_state;

    if (g_disp_data != RT_NULL)
    {
        rt_kprintf("waring: rt_display already initialed!\n");
        return g_disp_data;
    }

    rt_enter_critical();

    disp_data = (struct rt_display_data *)rt_malloc(sizeof(struct rt_display_data));
    RT_ASSERT(disp_data != RT_NULL);
    rt_memset((void *)disp_data, 0, sizeof(struct rt_display_data));

    device = rt_device_find("lcd");
    RT_ASSERT(device != RT_NULL);

    ret = rt_device_open(device, RT_DEVICE_OFLAG_RDWR);
    RT_ASSERT(ret == RT_EOK);

    disp_data->disp_mq = rt_mq_create("disp_mq", sizeof(struct rt_display_mq_t), 4, RT_IPC_FLAG_FIFO);
    RT_ASSERT(disp_data->disp_mq != RT_NULL);

    int path = SWITCH_TO_INTERNAL_DPHY;
    ret = rt_device_control(device, RK_DISPLAY_CTRL_MIPI_SWITCH, &path);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RK_DISPLAY_CTRL_ENABLE, NULL);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RK_DISPLAY_CTRL_AP_COP_MODE, (uint8_t *)1);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);
    memcpy(&disp_data->info, &info, sizeof(struct rt_device_graphic_info));

    /* load lut */
    memset(&lut_state, 0, sizeof(struct crtc_lut_state));

    if (lutA != RT_NULL)
    {
        lut_state.win_id = lutA->winId;
        lut_state.lut = lutA->lut;
        lut_state.lut_size = lutA->size;
        ret = rt_device_control(device, RK_DISPLAY_CTRL_LOAD_LUT, &lut_state);
        RT_ASSERT(ret == RT_EOK);
        disp_data->lut[lutA->winId].lut    = lutA->lut;
        disp_data->lut[lutA->winId].format = lutA->format;
    }

    if (lutB != RT_NULL)
    {
        lut_state.win_id = lutB->winId;
        lut_state.lut = lutB->lut;
        lut_state.lut_size = lutB->size;
        ret = rt_device_control(device, RK_DISPLAY_CTRL_LOAD_LUT, &lut_state);
        RT_ASSERT(ret == RT_EOK);
        disp_data->lut[lutB->winId].lut    = lutB->lut;
        disp_data->lut[lutB->winId].format = lutB->format;
    }

    if (lutC != RT_NULL)
    {
        lut_state.win_id = lutC->winId;
        lut_state.lut = lutC->lut;
        lut_state.lut_size = lutC->size;
        ret = rt_device_control(device, RK_DISPLAY_CTRL_LOAD_LUT, &lut_state);
        RT_ASSERT(ret == RT_EOK);
        disp_data->lut[lutC->winId].lut    = lutC->lut;
        disp_data->lut[lutC->winId].format = lutC->format;
    }

    disp_data->blval = rt_display_get_bl_max(device) / 2;
    ret = rt_device_control(device, RK_DISPLAY_CTRL_UPDATE_BL, &disp_data->blval);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(device, RK_DISPLAY_CTRL_COMMIT, NULL);
    RT_ASSERT(ret == RT_EOK);

    /* clear screen */
    ret = rt_display_screen_clear(device);
    RT_ASSERT(ret == RT_EOK);

    disp_data->xres = WIN_LAYERS_W;
    disp_data->yres = WIN_LAYERS_H;
    disp_data->device = device;

    g_disp_data = disp_data;

    rt_exit_critical();

    return disp_data;
}
RTM_EXPORT(rt_display_init);

/**
 * Get global display data struct.
 */
rt_display_data_t rt_display_get_disp(void)
{
    rt_uint32_t timeout = 200;

    while (--timeout != 0)
    {
        if (g_disp_data != RT_NULL)
        {
            return g_disp_data;
        }
        rt_thread_delay(1);
    };

    rt_kprintf("error: rt_display_get_disp fail!\n");
    return RT_NULL;
}
RTM_EXPORT(rt_display_get_disp);

/**
 * Display application deinitial, free resources.
 */
void rt_display_deinit(rt_display_data_t disp_data)
{
    rt_err_t ret;
    do
    {
        ret = rt_mq_delete(disp_data->disp_mq);
        rt_thread_delay(10);
    }
    while (ret != RT_EOK);

    while (1)
    {
        if (RT_EOK == rt_device_close(disp_data->device))
        {
            break;
        }
        rt_thread_delay(10);
    }

    rt_free(g_disp_data);
    g_disp_data = RT_NULL;
}
RTM_EXPORT(rt_display_deinit);

/*
 **************************************************************************************************
 *
 * rt display thread.
 *
 **************************************************************************************************
 */
/**
 * rt display thread.
 */
static void rt_display_thread(void *p)
{
    rt_err_t ret;
    struct rt_display_mq_t disp_mq;
    struct rt_display_config *winhead = RT_NULL;
    struct rt_display_lut lut0;
    struct rt_display_data *disp;

    /* init bpp_lut[256] */
    rt_display_update_lut(FORMAT_RGB_332);
    lut0.winId = 0;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    lut0.lut  = bpp_lut;
    lut0.size = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
    disp = rt_display_init(&lut0, 0, 0);
    RT_ASSERT(disp != RT_NULL);

    while (1)
    {
        ret = rt_mq_recv(disp->disp_mq, &disp_mq, sizeof(struct rt_display_mq_t), RT_WAITING_FOREVER);
        RT_ASSERT(ret == RT_EOK);

        rt_uint8_t i;
        winhead = RT_NULL;
        for (i = 0; i < 3; i++)
        {
            if (disp_mq.cfgsta & (0x01 << i))
            {
                rt_uint8_t winid;
                struct rt_display_lut *old_lut;
                struct rt_display_config *wincfg;

                wincfg  = &disp_mq.win[i];
                winid   = wincfg->winId;
                old_lut = &disp->lut[winid];
                if ((wincfg->format != old_lut->format) || (wincfg->lut != old_lut->lut))
                {
                    struct rt_display_lut  lut;
                    lut.winId  = winid;
                    lut.format = wincfg->format;
                    lut.lut    = wincfg->lut;
                    lut.size   = wincfg->lutsize;

                    if (winid == 0)
                        ret = rt_display_lutset(&lut, 0, 0);
                    else if (winid == 1)
                        ret = rt_display_lutset(0, &lut, 0);
                    else
                        ret = rt_display_lutset(0, 0, &lut);
                    RT_ASSERT(ret == RT_EOK);
                }

                rt_display_win_layers_list(&winhead, wincfg);
            }
        }

        ret = rt_display_win_layers_set(winhead);
        RT_ASSERT(ret == RT_EOK);

        if (disp_mq.disp_finish)
        {
            ret = disp_mq.disp_finish();
            RT_ASSERT(ret == RT_EOK);
        }
    }

    rt_display_deinit(disp);
    disp = RT_NULL;
}

/**
 * olpc clock demo application init.
 */
int rt_display_thread_init(void)
{
    rt_thread_t rtt_disp;

    rtt_disp = rt_thread_create("disp", rt_display_thread, RT_NULL, 2048, 3, 10);
    RT_ASSERT(rtt_disp != RT_NULL);
    rt_thread_startup(rtt_disp);

    return RT_EOK;
}
INIT_APP_EXPORT(rt_display_thread_init);
#endif
