/**
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************
 * @file    audio_resample_4ch.c
 * @author  Cherry Chen / Xing Zheng / XiaoTan Luo
 * @version V0.1
 * @date    16-12-2019
 * @brief   Resample 4 channels pcm data from 48k to 16k.
 ******************************************************************************/

#include "AudioConfig.h"
#include "filters/SRCFilters.h"

/**
 * resample init.
 * @param unsigned long ulInputRate  e.g: 48000.
 * @param unsigned long ulOutputRate e.g: 16000.
 * @return rt_err_t if init success, return RT_EOK.
 */
long resample_48to16_4ch_init(SRCState_4ch *pSRC, unsigned long ulInputRate, unsigned long ulOutputRate)
{
    long lNumPolyPhases, lSampleIncrement, lNumTaps;

    memset(pSRC->Left, 0, NUMTAPS_4CH * 2 * 4);
    pSRC->lastSample1 = 0;
    pSRC->lastSample2 = 0;
    pSRC->lastSample3 = 0;
    pSRC->lastSample4 = 0;
    switch ((ulOutputRate << 16) / ulInputRate)
    {
    //32k->16k
    case _32K_TO_16K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_0_0005_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_0_0005_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_0_0005_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_0_0005_16bit_2ch.sCoefs;
        break;
    }
    //44.1k->16k
    case _44K_TO_16K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_0_3628_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_0_3628_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_0_3628_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_0_3628_16bit_2ch.sCoefs;
        break;
    }
    //48k->16k
    case _48K_TO_16K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_0_3333_16bit_4ch.sNumPolyPhases;
        lNumTaps = SRCFilter_0_3333_16bit_4ch.sNumTaps;
        lSampleIncrement = SRCFilter_0_3333_16bit_4ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_0_3333_16bit_4ch.sCoefs;
        break;
    }
    // 8k->16k. 22k->44k
    case _8K_TO_16K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_2_0_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_2_0_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_2_0_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_2_0_16bit_2ch.sCoefs;
        break;
    }

    //22k->16k
    case _22K_TO_16K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_0_7256_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_0_7256_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_0_7256_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_0_7256_16bit_2ch.sCoefs;
        break;
    }
    //24k->16k
    case _24K_TO_16K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_0_6666_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_0_6666_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_0_6666_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_0_6666_16bit_2ch.sCoefs;
        break;
    }
    //11k->16k
    case _11K_TO_16K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_1_4512_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_1_4512_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_1_4512_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_1_4512_16bit_2ch.sCoefs;
        break;
    }
    //8k->44k
    case _8K_TO_44K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_5_5125_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_5_5125_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_5_5125_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_5_5125_16bit_2ch.sCoefs;
        break;
    }
    //11k->44k
    case _11K_TO_44K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_4_0000_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_4_0000_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_4_0000_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_4_0000_16bit_2ch.sCoefs;
        break;
    }
    //16k->44k
    case _16K_TO_44K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_2_7562_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_2_7562_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_2_7562_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_2_7562_16bit_2ch.sCoefs;
        break;
    }
    //32k->44k
    case _32K_TO_44K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_1_3781_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_1_3781_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_1_3781_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_1_3781_16bit_2ch.sCoefs;
        break;
    }
    //48k->44k
    case _48K_TO_44K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_0_9187_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_0_9187_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_0_9187_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_0_9187_16bit_2ch.sCoefs;
        break;
    }
    //8k->48k
    case _8K_TO_48K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_6_0000_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_6_0000_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_6_0000_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_6_0000_16bit_2ch.sCoefs;
        break;
    }
    //11k->48k
    case _11K_TO_48K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_4_3537_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_4_3537_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_4_3537_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_4_3537_16bit_2ch.sCoefs;
        break;
    }
    //16k->48k
    case _16K_TO_48K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_3_0000_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_3_0000_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_3_0000_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_3_0000_16bit_2ch.sCoefs;
        break;
    }
    //22k->48k
    case _22K_TO_48K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_2_1768_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_2_1768_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_2_1768_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_2_1768_16bit_2ch.sCoefs;
        break;
    }
    //32k->48k
    case _32K_TO_48K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_1_5000_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_1_5000_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_1_5000_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_1_5000_16bit_2ch.sCoefs;
        break;
    }
    //44k->48k
    case _44K_TO_48K:
    {
        RK_AUDIO_LOG_D("SRCInit :%d", __LINE__);
        lNumPolyPhases = SRCFilter_1_0884_16bit_2ch.sNumPolyPhases;
        lNumTaps = SRCFilter_1_0884_16bit_2ch.sNumTaps;
        lSampleIncrement = SRCFilter_1_0884_16bit_2ch.sSampleIncrement;
        pSRC->psFilter = (short *)SRCFilter_1_0884_16bit_2ch.sCoefs;
        break;
    }
    default:
    {
        return RK_AUDIO_FAILURE;
    }
    }
    /**
     * Make sure that the number of taps in the filter matches the number of
     * taps supported by our filtering code.
     */
    if (lNumTaps != NUMTAPS_4CH)
    {
        return RK_AUDIO_FAILURE;
    }
    /* Initialize the persistent SRC state. */
    pSRC->lFilterOffset = 0;
    pSRC->lFilterIncr = lSampleIncrement * NUMTAPS_4CH;
    pSRC->lFilterSize = lNumPolyPhases * NUMTAPS_4CH;

    return RK_AUDIO_SUCCESS;
}

/**
 * process resample.
 * @param ssrc_type *psInData1, input 4ch 16bit pcm data.
 * @param ssrc_type *psOutData1, output 4ch 16bit pcm data
 * and mute the 4th ch.
 * @param long lNumInputSamples, each channel's samples.
 * @param long plNumOutputSamples, each channel's samples.
 */
void resample_48to16_4ch_process(SRCState_4ch *pSRC, ssrc_type *psInData1, ssrc_type *psOutData1,
                                 long lNumInputSamples, long *plNumOutputSamples)
{
    long long lOutData1, lOutData2, lOutData3, lOutData4;
    short *psPtr1;
    ssrc_type  *psPtr2, *psPtr3, *psPtr4, *psPtr5;
    ssrc_type *psSample1, *psSample2, *psSample3, *psSample4;
    ssrc_type *psInData2 = psInData1 + 1;
    ssrc_type *psOutData2 = psOutData1 + 1;
    ssrc_type *psInData3 = psInData1 + 2;
    ssrc_type *psOutData3 = psOutData1 + 2;
    ssrc_type *psInData4 = psInData1 + 3;
    ssrc_type *psOutData4 = psOutData1 + 3;
    short sCoeff;
    int iLoop;
    int i = 0;
    static int count = 0;
    int tsize = sizeof(ssrc_type);

    /* Append the new data to the filter memory buffers. */
    memcpy(psInData1 - (NUMTAPS_4CH * 4), pSRC->Left, (NUMTAPS_4CH * 4 * tsize));

    /* Point to the last sample of the pre-existing audio data. */
    while (1)
    {
        psSample1 = psInData1 - 4 * i;
        psSample2 = psInData2 - 4 * i;
        psSample3 = psInData3 - 4 * i;
        psSample4 = psInData4 - 4 * i;
        i++;
        if (((*psSample1 == pSRC->lastSample1) &&
             (*psSample2 == pSRC->lastSample2) &&
             (*psSample3 == pSRC->lastSample3)) ||
            (i >= NUMTAPS_4CH))
        {
            break;
        }
    }

    /* Compute the number of output samples which we will produce. */
    iLoop = ((((lNumInputSamples + i - 1) * pSRC->lFilterSize) -
              pSRC->lFilterOffset - 1) / pSRC->lFilterIncr);

    *plNumOutputSamples = iLoop;

    /* Loop through each output sample. */
    while (iLoop--)
    {
        /* Increment the offset into the filter. */
        pSRC->lFilterOffset += pSRC->lFilterIncr;

        /**
         * See if we've passed the entire filter, indicating that we've
         * consumed one of the input samples.
         */
        while (pSRC->lFilterOffset >= pSRC->lFilterSize)
        {
            /**
             * We have, so wrap the filter offset (i.e. treat the filter as if
             * it were a circular buffer in memory).
             */
            pSRC->lFilterOffset -= pSRC->lFilterSize;
            /* Consume the input sample. */
            {
                psSample1 += 4;
                psSample2 += 4;
                psSample3 += 4;
                psSample4 += 4;
            }
        }
        /* Get pointers to the filter and the two sample data streams. */
        psPtr1 = pSRC->psFilter + pSRC->lFilterOffset;
        count++;

        psPtr2 = psSample1;
        psPtr3 = psSample2;
        psPtr4 = psSample3;
        psPtr5 = psSample4;

        /**
         * Run the filter over the two sample streams.  The number of MACs here
         * must agree with NUMTAPS.
         */
        lOutData1 = 0;
        lOutData2 = 0;
        lOutData3 = 0;
        lOutData4 = 0;
        sCoeff = *psPtr1++;
        i = 0;
        while (i < NUMTAPS_4CH)
        {
            lOutData1 += (long long)sCoeff * (long long)(*psPtr2);
            lOutData2 += (long long)sCoeff * (long long)(*psPtr3);
            lOutData3 += (long long)sCoeff * (long long)(*psPtr4);
            lOutData4 += (long long)sCoeff * (long long)(*psPtr5);
            if (i != (NUMTAPS_4CH - 1))
            {
                sCoeff = *psPtr1++;
                psPtr2 -= 4;
                psPtr3 -= 4;
                psPtr4 -= 4;
                psPtr5 -= 4;
            }
            i++;
        }

        /* Write out the samples. */
        *psOutData1 = (ssrc_type)(lOutData1 >> 15);
        psOutData1 += 4;

        *psOutData2 = (ssrc_type)(lOutData2 >> 15);
        psOutData2 += 4;

        *psOutData3 = (ssrc_type)(lOutData3 >> 15);
        psOutData3 += 4;

        *psOutData4 = (ssrc_type)(lOutData4 >> 15);
        psOutData4 += 4;
    }
    /**
     * Copy the tail of the filter memory buffer to the beginning, therefore
     * "remembering" the last few samples to use when processing the next batch
     * of samples.
     */
    pSRC->lastSample1 = *psSample1;
    pSRC->lastSample2 = *psSample2;
    pSRC->lastSample3 = *psSample3;
    pSRC->lastSample4 = *psSample4;

    memcpy(pSRC->Left, psInData1 + (lNumInputSamples * 4) - (NUMTAPS_4CH * 4), (NUMTAPS_4CH * 4 * tsize));
}
