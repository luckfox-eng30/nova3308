////////////////////////////////////////////////////////////////////////////////
///
/// Sampled sound tempo changer/time stretch algorithm. Changes the sound tempo
/// while maintaining the original pitch by using a time domain WSOLA-like
/// method with several performance-increasing tweaks.
///
/// Note : MMX optimized functions reside in a separate, platform-specific
/// file, e.g. 'mmx_win.cpp' or 'mmx_gcc.cpp'
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-04-01 22:49:30 +0300 (Sun, 01 Apr 2012) $
// File revision : $Revision: 1.12 $
//
// $Id: TDStretch.cpp 137 2012-04-01 19:49:30Z oparviai $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include "AudioConfig.h"
#include <float.h>

#include "STTypes.h"
#include "TDStretch.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))

/*****************************************************************************
 *
 * Constant definitions
 *
 *****************************************************************************/

// Table for the hierarchical mixing position seeking algorithm
static const short _scanOffsets[5][24] =
{
    {
        124,  186,  248,  310,  372,  434,  496,  558,  620,  682,  744, 806,
        868,  930,  992, 1054, 1116, 1178, 1240, 1302, 1364, 1426, 1488,   0
    },
    {
        -100,  -75,  -50,  -25,   25,   50,   75,  100,    0,    0,    0,   0,
            0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0
        },
    {
        -20,  -15,  -10,   -5,    5,   10,   15,   20,    0,    0,    0,   0,
            0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0
        },
    {
        -4,   -3,   -2,   -1,    1,    2,    3,    4,    0,    0,    0,   0,
            0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0
        },
    {
        121,  114,   97,  114,   98,  105,  108,   32,  104,   99,  117,  111,
        116,  100,  110,  117,  111,  115,    0,    0,    0,    0,    0,   0
    }
};

/*****************************************************************************
 *
 * Implementation of the class 'TDStretch'
 *
 *****************************************************************************/
void TDStretch_acceptNewOverlapLength(TDStretch *pTDS, int newOverlapLength);

void TDStretch_calculateOverlapLength(TDStretch *pTDS, int overlapMs);

double TDStretch_calcCrossCorr(TDStretch *pTDS, const SAMPLETYPE *mixingPos, const SAMPLETYPE *compare) ;

int TDStretch_seekBestOverlapPosition(TDStretch *pTDS, const SAMPLETYPE *refPos);

void TDStretch_overlapStereo(TDStretch *pTDS, SAMPLETYPE *output, const SAMPLETYPE *input);
void TDStretch_overlapMono(TDStretch *pTDS, SAMPLETYPE *output, const SAMPLETYPE *input);

void TDStretch_clearMidBuffer(TDStretch *pTDS);
void TDStretch_overlap(TDStretch *pTDS, SAMPLETYPE *output, const SAMPLETYPE *input, uint ovlPos);

void TDStretch_calcSeqParameters(TDStretch *pTDS);

/// Changes the tempo of the given sound samples.
/// Returns amount of samples returned in the "output" buffer.
/// The maximum amount of samples that can be returned at a time is set by
/// the 'set_returnBuffer_size' function.
void processSamples(TDStretch *pTDS);

#define MID_SIZE 544
SAMPLETYPE midBuf[MID_SIZE];

void TDStretch_init(TDStretch *pTDS) /*: FIFOProcessor(&outputBuffer)*/
{
    pTDS->channels = 2;
    FIFOSampleBuffer_init(&(pTDS->outputBuffer), pTDS->channels);
    FIFOSampleBuffer_init(&(pTDS->inputBuffer), pTDS->channels);
    pTDS->pMidBuffer = NULL;
    pTDS->pMidBufferUnaligned = NULL;
    pTDS->overlapLength = 0;

    pTDS->bAutoSeqSetting = RK_AUDIO_TRUE;
    pTDS->bAutoSeekSetting = RK_AUDIO_TRUE;

//    outDebt = 0;
    pTDS->skipFract = 0;

    pTDS->tempo = 1.0f;
    TDStretch_setParameters(pTDS, 44100, DEFAULT_SEQUENCE_MS, DEFAULT_SEEKWINDOW_MS, DEFAULT_OVERLAP_MS);
    TDStretch_setTempo(pTDS, 1.0f);

    TDStretch_clear(pTDS);
}



void TDStretch_deInit(TDStretch *pTDS)
{
    FIFOSampleBuffer_deInit(&(pTDS->outputBuffer));
    FIFOSampleBuffer_deInit(&(pTDS->inputBuffer));
    //free(pTDS->pMidBufferUnaligned);
}

/// Returns the output buffer object
FIFOSampleBuffer *TDStretch_getOutput(TDStretch *pTDS)
{
    return &(pTDS->outputBuffer);
}

/// Returns the input buffer object
FIFOSampleBuffer *TDStretch_getInput(TDStretch *pTDS)
{
    return &(pTDS->inputBuffer);
}


// Sets routine control parameters. These control are certain time constants
// defining how the sound is stretched to the desired duration.
//
// 'sampleRate' = sample rate of the sound
// 'sequenceMS' = one processing sequence length in milliseconds (default = 82 ms)
// 'seekwindowMS' = seeking window length for scanning the best overlapping
//      position (default = 28 ms)
// 'overlapMS' = overlapping length (default = 12 ms)

void TDStretch_setParameters(TDStretch *pTDS, int aSampleRate, int aSequenceMS,
                             int aSeekWindowMS, int aOverlapMS)
{
    // accept only positive parameter values - if zero or negative, use old values instead
    if (aSampleRate > 0)   pTDS->sampleRate = aSampleRate;
    if (aOverlapMS > 0)    pTDS->overlapMs = aOverlapMS;

    if (aSequenceMS > 0)
    {
        pTDS->sequenceMs = aSequenceMS;
        pTDS->bAutoSeqSetting = RK_AUDIO_FALSE;
    }
    else if (aSequenceMS == 0)
    {
        // if zero, use automatic setting
        pTDS->bAutoSeqSetting = RK_AUDIO_TRUE;
    }

    if (aSeekWindowMS > 0)
    {
        pTDS->seekWindowMs = aSeekWindowMS;
        pTDS->bAutoSeekSetting = RK_AUDIO_FALSE;
    }
    else if (aSeekWindowMS == 0)
    {
        // if zero, use automatic setting
        pTDS->bAutoSeekSetting = RK_AUDIO_TRUE;
    }

    TDStretch_calcSeqParameters(pTDS);

    TDStretch_calculateOverlapLength(pTDS, pTDS->overlapMs);

    // set tempo to recalculate 'sampleReq'
    TDStretch_setTempo(pTDS, pTDS->tempo);

}



/// Get routine control parameters, see setParameters() function.
/// Any of the parameters to this function can be NULL, in such case corresponding parameter
/// value isn't returned.
void TDStretch_getParameters(TDStretch *pTDS, int *pSampleRate, int *pSequenceMs, int *pSeekWindowMs, int *pOverlapMs)
{
    if (pSampleRate)
    {
        *pSampleRate = pTDS->sampleRate;
    }

    if (pSequenceMs)
    {
        *pSequenceMs = (pTDS->bAutoSeqSetting) ? (USE_AUTO_SEQUENCE_LEN) : pTDS->sequenceMs;
    }

    if (pSeekWindowMs)
    {
        *pSeekWindowMs = (pTDS->bAutoSeekSetting) ? (USE_AUTO_SEEKWINDOW_LEN) : pTDS->seekWindowMs;
    }

    if (pOverlapMs)
    {
        *pOverlapMs = pTDS->overlapMs;
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'pInput'
void TDStretch_overlapMono(TDStretch *pTDS, SAMPLETYPE *pOutput, const SAMPLETYPE *pInput)
{
    int i;
    SAMPLETYPE m1, m2;

    m1 = (SAMPLETYPE)0;
    m2 = (SAMPLETYPE)pTDS->overlapLength;

    for (i = 0; i < pTDS->overlapLength ; i ++)
    {
        pOutput[i] = (pInput[i] * m1 + pTDS->pMidBuffer[i] * m2) / pTDS->overlapLength;
        m1 += 1;
        m2 -= 1;
    }
}



void TDStretch_clearMidBuffer(TDStretch *pTDS)
{
    memset(pTDS->pMidBuffer, 0, 2 * sizeof(SAMPLETYPE) * pTDS->overlapLength);
}


void TDStretch_clearInput(TDStretch *pTDS)
{
    //inputBuffer.clear();
    FIFOSampleBuffer_clear(&(pTDS->inputBuffer));
    TDStretch_clearMidBuffer(pTDS);
}


// Clears the sample buffers
void TDStretch_clear(TDStretch *pTDS)
{
    //outputBuffer.clear();
    FIFOSampleBuffer_clear(&(pTDS->outputBuffer));
    TDStretch_clearInput(pTDS);
}

// Seeks for the optimal overlap-mixing position.


// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period

int TDStretch_seekBestOverlapPosition(TDStretch *pTDS, const SAMPLETYPE *refPos)
{
    int bestOffs;
    double bestCorr, corr;
    int scanCount, corrOffset, tempOffset;

    bestCorr = FLT_MIN;
    bestOffs = _scanOffsets[0][0];
    corrOffset = 0;
    tempOffset = 0;

    // Scans for the best correlation value using four-pass hierarchical search.
    //
    // The look-up table 'scans' has hierarchical position adjusting steps.
    // In first pass the routine searhes for the highest correlation with
    // relatively coarse steps, then rescans the neighbourhood of the highest
    // correlation with better resolution and so on.
    for (scanCount = 0; scanCount < 4; scanCount ++)
    {
        int j = 0;
        while (_scanOffsets[scanCount][j])
        {
            tempOffset = corrOffset + _scanOffsets[scanCount][j];
            if (tempOffset >= pTDS->seekLength) break;

            // Calculates correlation value for the mixing position corresponding
            // to 'tempOffset'
            corr = (double)TDStretch_calcCrossCorr(pTDS, refPos + pTDS->channels * tempOffset, pTDS->pMidBuffer);
            // heuristic rule to slightly favour values close to mid of the range
            {
                double tmp = (double)(2 * tempOffset - pTDS->seekLength) / pTDS->seekLength;
                corr = ((corr + 0.1) * (1.0 - 0.25 * tmp * tmp));
            }
            // Checks for the highest correlation value
            if (corr > bestCorr)
            {
                bestCorr = corr;
                bestOffs = tempOffset;
            }
            j ++;
        }
        corrOffset = bestOffs;
    }

    return bestOffs;
}


// Overlaps samples in 'midBuffer' with the samples in 'pInputBuffer' at position
// of 'ovlPos'.
void TDStretch_overlap(TDStretch *pTDS, SAMPLETYPE *pOutput, const SAMPLETYPE *pInput, uint ovlPos)
{
    if (pTDS->channels == 2)
    {
        // stereo sound
        TDStretch_overlapStereo(pTDS, pOutput, pInput + 2 * ovlPos);
    }
    else
    {
        // mono sound.
        TDStretch_overlapMono(pTDS, pOutput, pInput + ovlPos);
    }
}

/// Calculates processing sequence length according to tempo setting
void TDStretch_calcSeqParameters(TDStretch *pTDS)
{
    // Adjust tempo param according to tempo, so that variating processing sequence length is used
    // at varius tempo settings, between the given low...top limits
#define AUTOSEQ_TEMPO_LOW   0.5     // auto setting low tempo range (-50%)
#define AUTOSEQ_TEMPO_TOP   2.0     // auto setting top tempo range (+100%)

    // sequence-ms setting values at above low & top tempo
#define AUTOSEQ_AT_MIN      125.0
#define AUTOSEQ_AT_MAX      50.0
#define AUTOSEQ_K           ((AUTOSEQ_AT_MAX - AUTOSEQ_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
#define AUTOSEQ_C           (AUTOSEQ_AT_MIN - (AUTOSEQ_K) * (AUTOSEQ_TEMPO_LOW))

    // seek-window-ms setting values at above low & top tempo
#define AUTOSEEK_AT_MIN     25.0
#define AUTOSEEK_AT_MAX     15.0
#define AUTOSEEK_K          ((AUTOSEEK_AT_MAX - AUTOSEEK_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW))
#define AUTOSEEK_C          (AUTOSEEK_AT_MIN - (AUTOSEEK_K) * (AUTOSEQ_TEMPO_LOW))

#define CHECK_LIMITS(x, mi, ma) (((x) < (mi)) ? (mi) : (((x) > (ma)) ? (ma) : (x)))


    if (pTDS->bAutoSeqSetting)
    {
        double seq = AUTOSEQ_C + AUTOSEQ_K * pTDS->tempo;
        seq = CHECK_LIMITS(seq, AUTOSEQ_AT_MAX, AUTOSEQ_AT_MIN);
        pTDS->sequenceMs = (int)(seq + 0.5);
    }

    if (pTDS->bAutoSeekSetting)
    {
        double seek = AUTOSEEK_C + AUTOSEEK_K * pTDS->tempo;
        seek = CHECK_LIMITS(seek, AUTOSEEK_AT_MAX, AUTOSEEK_AT_MIN);
        pTDS->seekWindowMs = (int)(seek + 0.5);
    }

    // Update seek window lengths
    pTDS->seekWindowLength = (pTDS->sampleRate * pTDS->sequenceMs) / 1000;
    if (pTDS->seekWindowLength < 2 * pTDS->overlapLength)
    {
        pTDS->seekWindowLength = 2 * pTDS->overlapLength;
    }
    pTDS->seekLength = (pTDS->sampleRate * pTDS->seekWindowMs) / 1000;
}



// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower
// tempo, larger faster tempo.
void TDStretch_setTempo(TDStretch *pTDS, float newTempo)
{
    int intskip;

    pTDS->tempo = newTempo;

    // Calculate new sequence duration
    TDStretch_calcSeqParameters(pTDS);

    // Calculate ideal skip length (according to tempo value)
    pTDS->nominalSkip = pTDS->tempo * (pTDS->seekWindowLength - pTDS->overlapLength);
    intskip = (int)(pTDS->nominalSkip + 0.5f);

    // Calculate how many samples are needed in the 'inputBuffer' to
    // process another batch of samples
    //sampleReq = max(intskip + overlapLength, seekWindowLength) + seekLength / 2;
    pTDS->sampleReq = max(intskip + pTDS->overlapLength, pTDS->seekWindowLength) + pTDS->seekLength;
}



// Sets the number of channels, 1 = mono, 2 = stereo
void TDStretch_setChannels(TDStretch *pTDS, int numChannels)
{
    if (pTDS->channels == numChannels) return;

    pTDS->channels = numChannels;
    //inputBuffer.setChannels(channels);
    FIFOSampleBuffer_setChannels(&(pTDS->inputBuffer), pTDS->channels);
    //outputBuffer.setChannels(channels);
    FIFOSampleBuffer_setChannels(&(pTDS->outputBuffer), pTDS->channels);
}



#include <stdio.h>

// Processes as many processing frames of the samples 'inputBuffer', store
// the result into 'outputBuffer'
void TDStretch_processSamples(TDStretch *pTDS)
{
    int ovlSkip;

    // Process samples as long as there are enough samples in 'inputBuffer'
    // to form a processing frame.
    // while ((int)inputBuffer.numSamples() >= sampleReq)
    while ((int)FIFOSampleBuffer_numSamples(&(pTDS->inputBuffer)) >= pTDS->sampleReq)
    {
        // If tempo differs from the normal ('SCALE'), scan for the best overlapping
        // position
        int offset = TDStretch_seekBestOverlapPosition(pTDS, FIFOSampleBuffer_ptrBegin(&(pTDS->inputBuffer)));

        // Mix the samples in the 'inputBuffer' at position of 'offset' with the
        // samples in 'midBuffer' using sliding overlapping
        // ... first partially overlap with the end of the previous sequence
        // (that's in 'midBuffer')
        TDStretch_overlap(pTDS, FIFOSampleBuffer_ptrEnd(&(pTDS->outputBuffer), (uint)pTDS->overlapLength), FIFOSampleBuffer_ptrBegin(&(pTDS->inputBuffer)), (uint)offset);
        FIFOSampleBuffer_putSamples(&(pTDS->outputBuffer), (uint)pTDS->overlapLength);

        // ... then copy sequence samples from 'inputBuffer' to output:

        // length of sequence
        int temp = (pTDS->seekWindowLength - 2 * pTDS->overlapLength);

        // crosscheck that we don't have buffer overflow...
        if ((int)FIFOSampleBuffer_numSamples(&(pTDS->inputBuffer)) < (offset + temp + pTDS->overlapLength * 2))
        {
            continue;    // just in case, shouldn't really happen
        }

        FIFOSampleBuffer_putSamples2(&(pTDS->outputBuffer), FIFOSampleBuffer_ptrBegin(&(pTDS->inputBuffer)) + pTDS->channels * (offset + pTDS->overlapLength), (uint)temp);

        // Copies the end of the current sequence from 'inputBuffer' to
        // 'midBuffer' for being mixed with the beginning of the next
        // processing sequence and so on
        memcpy(pTDS->pMidBuffer, FIFOSampleBuffer_ptrBegin(&(pTDS->inputBuffer)) + pTDS->channels * (offset + temp + pTDS->overlapLength),
               pTDS->channels * sizeof(SAMPLETYPE) * pTDS->overlapLength);

        // Remove the processed samples from the input buffer. Update
        // the difference between integer & nominal skip step to 'skipFract'
        // in order to prevent the error from accumulating over time.
        pTDS->skipFract += pTDS->nominalSkip;   // real skip size
        ovlSkip = (int)pTDS->skipFract;   // rounded to integer skip
        pTDS->skipFract -= ovlSkip;       // maintain the fraction part, i.e. real vs. integer skip
        FIFOSampleBuffer_receiveSamples(&(pTDS->inputBuffer), (uint)ovlSkip); //inputBuffer.receiveSamples((uint)ovlSkip);
    }
}


// Adds 'numsamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void TDStretch_putSamples(TDStretch *pTDS, const SAMPLETYPE *samples, uint nSamples)
{
    //printf("a");
    // Add the samples into the input buffer
    FIFOSampleBuffer_putSamples2(&(pTDS->inputBuffer), samples, nSamples);
    // Process the samples in input buffer
    //printf("c");
    TDStretch_processSamples(pTDS);
    //printf("b");
}

void TDStretch_moveSamples(TDStretch *pTDS, FIFOSampleBuffer *other)    ///< Other pipe instance where from the receive the data.
{
    int oNumSamples = FIFOSampleBuffer_numSamples(other);//other.numSamples();

    //putSamples(other.ptrBegin(), oNumSamples);
    TDStretch_putSamples(pTDS, FIFOSampleBuffer_ptrBegin(other), oNumSamples);
    //other.receiveSamples(oNumSamples);
    FIFOSampleBuffer_receiveSamples(other, oNumSamples);
}

/// return nominal input sample requirement for triggering a processing batch
int TDStretch_getInputSampleReq(TDStretch *pTDS)
{
    return (int)(pTDS->nominalSkip + 0.5);
}

/// return nominal output sample amount when running a processing batch
int TDStretch_getOutputBatchSize(TDStretch *pTDS)
{
    return pTDS->seekWindowLength - pTDS->overlapLength;
}


/// Set new overlap length parameter & reallocate RefMidBuffer if necessary.
void TDStretch_acceptNewOverlapLength(TDStretch *pTDS, int newOverlapLength)
{
    int prevOvl;

    prevOvl = pTDS->overlapLength;
    pTDS->overlapLength = newOverlapLength;

    if (pTDS->overlapLength > prevOvl)
    {
        //free(pTDS->pMidBufferUnaligned);
        if (pTDS->overlapLength * 2 * sizeof(SAMPLETYPE) + 16 <= MID_SIZE * 2)
            pTDS->pMidBufferUnaligned = midBuf;
        //pTDS->pMidBufferUnaligned = (SAMPLETYPE *)malloc(pTDS->overlapLength * 2*sizeof(SAMPLETYPE)+16);
        // ensure that 'pMidBuffer' is aligned to 16 byte boundary for efficiency
        pTDS->pMidBuffer = (SAMPLETYPE *)((((ulong)pTDS->pMidBufferUnaligned) + 15) & (ulong) - 16);

        TDStretch_clearMidBuffer(pTDS);
    }
}



// Overlaps samples in 'midBuffer' with the samples in 'input'. The 'Stereo'
// version of the routine.
void TDStretch_overlapStereo(TDStretch *pTDS, short *poutput, const short *input)
{
    int i;

    for (i = 0; i < pTDS->overlapLength ; i ++)
    {
        short temp = (short)(pTDS->overlapLength - i);
        int cnt2 = 2 * i;
        poutput[cnt2] = (input[cnt2] * i + pTDS->pMidBuffer[cnt2] * temp)  / pTDS->overlapLength;
        poutput[cnt2 + 1] = (input[cnt2 + 1] * i + pTDS->pMidBuffer[cnt2 + 1] * temp) / pTDS->overlapLength;
    }
}

// Calculates the x having the closest 2^x value for the given value
static int _getClosest2Power(double value)
{
    return (int)(log(value) / log(2.0) + 0.5);
}


/// Calculates overlap period length in samples.
/// Integer version rounds overlap length to closest power of 2
/// for a divide scaling operation.
void TDStretch_calculateOverlapLength(TDStretch *pTDS, int aoverlapMs)
{
    int newOvl;

    // calculate overlap length so that it's power of 2 - thus it's easy to do
    // integer division by right-shifting. Term "-1" at end is to account for
    // the extra most significatnt bit left unused in result by signed multiplication
    pTDS->overlapDividerBits = _getClosest2Power((pTDS->sampleRate * aoverlapMs) / 1000.0) - 1;
    if (pTDS->overlapDividerBits > 9) pTDS->overlapDividerBits = 9;
    if (pTDS->overlapDividerBits < 3) pTDS->overlapDividerBits = 3;
    newOvl = (int)pow(2.0, (int)pTDS->overlapDividerBits + 1);    // +1 => account for -1 above
    TDStretch_acceptNewOverlapLength(pTDS, newOvl);

    // calculate sloping divider so that crosscorrelation operation won't
    // overflow 32-bit register. Max. sum of the crosscorrelation sum without
    // divider would be 2^30*(N^3-N)/3, where N = overlap length
    pTDS->slopingDivider = (newOvl * newOvl - 1) / 3;
}


double TDStretch_calcCrossCorr(TDStretch *pTDS, const short *mixingPos, const short *compare)
{
    long corr;
    long norm;
    int i;

    corr = norm = 0;
    // Same routine for stereo and mono. For stereo, unroll loop for better
    // efficiency and gives slightly better resolution against rounding.
    // For mono it same routine, just  unrolls loop by factor of 4
    for (i = 0; i < pTDS->channels * pTDS->overlapLength; i += 4)
    {
        corr += (mixingPos[i] * compare[i] +
                 mixingPos[i + 1] * compare[i + 1] +
                 mixingPos[i + 2] * compare[i + 2] +
                 mixingPos[i + 3] * compare[i + 3]) >> pTDS->overlapDividerBits;
        norm += (mixingPos[i] * mixingPos[i] +
                 mixingPos[i + 1] * mixingPos[i + 1] +
                 mixingPos[i + 2] * mixingPos[i + 2] +
                 mixingPos[i + 3] * mixingPos[i + 3]) >> pTDS->overlapDividerBits;
    }

    // Normalize result by dividing by sqrt(norm) - this step is easiest
    // done using floating point operation
    if (norm == 0) norm = 1;    // to avoid div by zero
    return (double)corr / sqrt((double)norm);
}



