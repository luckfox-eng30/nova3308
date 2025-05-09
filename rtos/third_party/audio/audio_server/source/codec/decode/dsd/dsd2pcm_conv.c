#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "AudioConfig.h"
#include "dsd2pcm_conv.h"
/*
* The 435-tap symmetric lowpass filter: 20 kHz, 140 dB
*/

/*
_ATTR_DSDIFFDEC_TEXT_
const static
*/

const static int32_t htaps_315[] = //FIR order = 512¡ê?winodws:Chebyshe
{
    -41, -37, -55, -78, -107,
        -144, -190, -247, -316, -400,
        -501, -621, -764, -931, -1127,
        -1354, -1615, -1916, -2259, -2648,
        -3087, -3581, -4133, -4748, -5428,
        -6178, -7000, -7897, -8870, -9920,
        -11048, -12252, -13530, -14877, -16289,
        -17755, -19267, -20812, -22372, -23930,
        -25462, -26941, -28337, -29613, -30729,
        -31637, -32285, -32615, -32561, -32049,
        -30999, -29322, -26921, -23689, -19512,
        -14263, -7808, 0, 9317, 20311,
        33160, 48055, 65197, 84800, 107089,
        132300, 160682, 192494, 228008, 267504,
        311275, 359624, 412862, 471309, 535295,
        605154, 681230, 763871, 853427, 950255,
        1054711, 1167153, 1287937, 1417418, 1555946,
        1703865, 1861512, 2029214, 2207289, 2396041,
        2595758, 2806712, 3029156, 3263324, 3509424,
        3767642, 4038135, 4321034, 4616438, 4924413,
        5244991, 5578171, 5923910, 6282131, 6652714,
        7035497, 7430277, 7836807, 8254796, 8683908,
        9123760, 9573927, 10033934, 10503263, 10981351,
        11467588, 11961321, 12461855, 12968452, 13480333,
        13996681, 14516642, 15039326, 15563811, 16089142,
        16614341, 17138399, 17660289, 18178963, 18693358,
        19202399, 19705000, 20200071, 20686521, 21163260,
        21629204, 22083280, 22524427, 22951603, 23363786,
        23759982, 24139222, 24500574, 24843139, 25166059,
        25468521, 25749755, 26009044, 26245720, 26459173,
        26648848, 26814251, 26954950, 27070576, 27160825,
        27225459, 27264308, 27277269, 27264308, 27225459,
        27160825, 27070576, 26954950, 26814251, 26648848,
        26459173, 26245720, 26009044, 25749755, 25468521,
        25166059, 24843139, 24500574, 24139222, 23759982,
        23363786, 22951603, 22524427, 22083280, 21629204,
        21163260, 20686521, 20200071, 19705000, 19202399,
        18693358, 18178963, 17660289, 17138399, 16614341,
        16089142, 15563811, 15039326, 14516642, 13996681,
        13480333, 12968452, 12461855, 11961321, 11467588,
        10981351, 10503263, 10033934, 9573927, 9123760,
        8683908, 8254796, 7836807, 7430277, 7035497,
        6652714, 6282131, 5923910, 5578171, 5244991,
        4924413, 4616438, 4321034, 4038135, 3767642,
        3509424, 3263324, 3029156, 2806712, 2595758,
        2396041, 2207289, 2029214, 1861512, 1703865,
        1555946, 1417418, 1287937, 1167153, 1054711,
        950255, 853427, 763871, 681230, 605154,
        535295, 471309, 412862, 359624, 311275,
        267504, 228008, 192494, 160682, 132300,
        107089, 84800, 65197, 48055, 33160,
        20311, 9317, 0, -7808, -14263,
        -19512, -23689, -26921, -29322, -30999,
        -32049, -32561, -32615, -32285, -31637,
        -30729, -29613, -28337, -26941, -25462,
        -23930, -22372, -20812, -19267, -17755,
        -16289, -14877, -13530, -12252, -11048,
        -9920, -8870, -7897, -7000, -6178,
        -5428, -4748, -4133, -3581, -3087,
        -2648, -2259, -1916, -1615, -1354,
        -1127, -931, -764, -621, -501,
        -400, -316, -247, -190, -144,
        -107, -78, -55, -37, -41
    };

const static int32_t htaps_20000[] =
{
    19080, 22079, 34559, 51432, 73712,
    102557, 139274, 185326, 242334, 312076,
    396490, 497669, 617851, 759411, 924852,
    1116779, 1337890, 1590945, 1878744, 2204093,
    2569778, 2978521, 3432952, 3935561, 4488661,
    5094346, 5754445, 6470483, 7243637, 8074696,
    8964024, 9911520, 10916592, 11978126, 13094459,
    14263370, 15482060, 16747150, 18054680, 19400120,
    20778383, 22183849, 23610394, 25051426, 26499934,
    27948533, 29389526, 30814962, 32216708, 33586520,
    34916116, 36197256, 37421822, 38581897, 39669843,
    40678382, 41600671, 42430370, 43161715, 43789576,
    44309513, 44717826, 45011592, 45188703, 45247882,
    45188703, 45011592, 44717826, 44309513, 43789576,
    43161715, 42430370, 41600671, 40678382, 39669843,
    38581897, 37421822, 36197256, 34916116, 33586520,
    32216708, 30814962, 29389526, 27948533, 26499934,
    25051426, 23610394, 22183849, 20778383, 19400120,
    18054680, 16747150, 15482060, 14263370, 13094459,
    11978126, 10916592, 9911520, 8964024, 8074696,
    7243637, 6470483, 5754445, 5094346, 4488661,
    3935561, 3432952, 2978521, 2569778, 2204093,
    1878744, 1590945, 1337890, 1116779, 924852,
    759411, 617851, 497669, 396490, 312076,
    242334, 185326, 139274, 102557, 73712,
    51432, 34559, 22079, 19080
};

/*It's an implementation of a symmetric 435-taps FIR lowpass filter
optimized for DSD inputs. */
static void dsd2pcm_converter_preinit(dsd2pcm_converter_t *dsd2pcm, const int32_t *htaps, int order)
{
    int ct, i, j, k;
    int coef;
    dsd2pcm->ctable_len = (order + 7) / 8;
    for (ct = 0; ct < dsd2pcm->ctable_len; ct++)
    {
        k = order - ct * 8;
        if (k > 8)
            k = 8;
        for (i = 0; i < 256; i++)
        {
            coef = 0;
            for (j = 0; j < k; j++)
            {
                coef += (((i >> j) & 1) * 2 - 1) * htaps[ct * 8 + j];
            }
            dsd2pcm->ctables[i][ct] = coef;
        }
    }
}

void dsd2pcm_converter_set_gain(dsd2pcm_converter_t *dsd2pcm, real_t dB_gain)
{
    dsd2pcm->gain = (real_t)(4.66e-10 * sqrt(pow(10.0, dB_gain / 20.0)));
}

void *dsd2pcm_converter_init(dec_type type, int channels, int dsd_samplerate, int pcm_samplerate)
{
    dsd2pcm_converter_t *dsd2pcm;
    int ch, i;

    dsd2pcm = audio_malloc(sizeof(dsd2pcm_converter_t));
    if (!dsd2pcm)
        return NULL;
    memset(dsd2pcm, 0x0, sizeof(dsd2pcm_converter_t));
    if (dsd2pcm->preinit != pcm_samplerate)
    {
        switch (type)
        {
        case DSD128:  //5.6M || 11.2M
        case DSD256:
            dsd2pcm_converter_preinit(dsd2pcm, htaps_315, 315);
            break;
        default:
            dsd2pcm_converter_preinit(dsd2pcm, htaps_20000, 129);
            break;
        }
        dsd2pcm->preinit = pcm_samplerate;
    }
    dsd2pcm->channels = channels;
    dsd2pcm->dsd_samplerate = dsd_samplerate;
    dsd2pcm->pcm_samplerate = pcm_samplerate;
    dsd2pcm->decimation = dsd_samplerate / pcm_samplerate >> 3;
    for (ch = 0; ch < dsd2pcm->channels; ch++)
    {
        for (i = 0; i < FIFOSIZE; i++)
        {
            dsd2pcm->ch[ch].fifo[i] = 0x00;
        }
        dsd2pcm->ch[ch].fifopos = FIFOMASK;
    }
    dsd2pcm_converter_set_gain(dsd2pcm, (real_t)0);

    return (void *)dsd2pcm;
}

void dsd2pcm_converter_deinit(dsd2pcm_converter_t *dsd2pcm)
{
    if (dsd2pcm)
        audio_free(dsd2pcm);
}

int dsd2pcm_converter_convert(dsd2pcm_converter_t *dsd2pcm, uint8_t *dsd_data, uint32_t dsd_size, uint8_t *pcm_out, uint16_t bps, int dsf)
{
    int ch, i, ct;
    uint8_t dsd_byte;
    int pcm_sample;
    int pcm_samples;
    int *pcm_ptr;
    int channels = dsd2pcm->channels;

    bps = 32;

    pcm_samples = dsd_size / dsd2pcm->decimation;

    if (dsf)
    {
        for (ch = 0; ch < channels; ch++)
        {

            uint8_t *fifo = dsd2pcm->ch[ch].fifo;
            uint32_t fifopos = dsd2pcm->ch[ch].fifopos;

            /* setup pcm pointer */
            pcm_ptr = (int *)(pcm_out + ch * (bps >> 3));

            /* filter one channel */
            for (i = 0; i < dsd_size / channels; i++)
            {

                /* get incoming DSD byte */
                dsd_byte = dsd_data[i + ch * (dsd_size / channels)];
                /* put incoming DSD byte into queue, advance queue */
                fifopos = (fifopos + 1) & FIFOMASK;
                fifo[fifopos] = dsd_byte;

                /* filter out on each pcm sample */
                if ((i % dsd2pcm->decimation) == 0)
                {
                    pcm_sample = 0;
                    for (ct = 0; ct < dsd2pcm->ctable_len; ct++)
                    {
                        dsd_byte = fifo[(fifopos + ct) & FIFOMASK];
                        pcm_sample += dsd2pcm->ctables[dsd_byte][ct];
                    }
                    pcm_sample = MAX(pcm_sample, -2147483648);  // CLIP < 32768
                    pcm_sample = MIN(pcm_sample, 2147483647);   // CLIP > 32767
                    *pcm_ptr++ = pcm_sample;
                    pcm_ptr++;
                }
                dsd2pcm->ch[ch].fifopos = fifopos;
            }
        }
    }
    else
    {
        for (ch = 0; ch < channels; ch++)
        {

            uint8_t *fifo = dsd2pcm->ch[ch].fifo;
            uint32_t fifopos = dsd2pcm->ch[ch].fifopos;

            /* setup pcm pointer */
            pcm_ptr = (int *)(pcm_out + ch * (bps >> 3));

            /* filter one channel */
            for (i = 0; i < dsd_size / channels; i++)
            {

                /* get incoming DSD byte */
                dsd_byte = dsd_data[i * channels + ch];
                /* put incoming DSD byte into queue, advance queue */
                fifopos = (fifopos + 1) & FIFOMASK;
                fifo[fifopos] = dsd_byte;

                /* filter out on each pcm sample */
                if ((i % dsd2pcm->decimation) == 0)
                {
                    pcm_sample = 0;
                    for (ct = 0; ct < dsd2pcm->ctable_len; ct++)
                    {
                        dsd_byte = fifo[(fifopos - ct) & FIFOMASK];
                        pcm_sample += dsd2pcm->ctables[dsd_byte][ct];
                    }
                    pcm_sample = MAX(pcm_sample, -2147483648);  // CLIP < 32768
                    pcm_sample = MIN(pcm_sample, 2147483647);   // CLIP > 32767
                    *pcm_ptr++ = pcm_sample;
                    pcm_ptr++;
                }
                dsd2pcm->ch[ch].fifopos = fifopos;
            }
        }
    }

    return pcm_samples;
}

