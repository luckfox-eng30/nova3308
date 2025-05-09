////////////////////////////////////////////////////////////////////////////////
///
/// Sample rate transposer. Changes sample rate by using linear interpolation
/// together with anti-alias filtering (first order interpolation with anti-
/// alias filtering should be quite adequate for this application)
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2011-09-02 21:56:11 +0300 (Fri, 02 Sep 2011) $
// File revision : $Revision: 4 $
//
// $Id: RateTransposer.cpp 131 2011-09-02 18:56:11Z oparviai $
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

//#include <memory.h>
#include "AudioConfig.h"
#include "RateTransposer.h"

//the follow method is private method.
void RateTransposer_resetRegisters(RateTransposer *pRT);

uint RateTransposer_transposeStereo(RateTransposer *pRT, SAMPLETYPE *dest,
                                    const SAMPLETYPE *src,
                                    uint numSamples) ;
uint RateTransposer_transposeMono(RateTransposer *pRT, SAMPLETYPE *dest,
                                  const SAMPLETYPE *src,
                                  uint numSamples);
uint RateTransposer_transpose(RateTransposer *pRT, SAMPLETYPE *dest,
                              const SAMPLETYPE *src,
                              uint numSamples);

/// Transposes sample rate by applying anti-alias filter to prevent folding.
/// Returns amount of samples returned in the "dest" buffer.
/// The maximum amount of samples that can be returned at a time is set by
/// the 'set_returnBuffer_size' function.
void RateTransposer_processSamples(RateTransposer *pRT, const SAMPLETYPE *src,
                                   uint numSamples);



// Constructor
void RateTransposer_init(RateTransposer *pRT) /*: FIFOProcessor(&outputBuffer)*/
{

    pRT->numChannels = 2;
    pRT->fRate = 0;
    RateTransposer_resetRegisters(pRT);
    RateTransposer_setRate(pRT, 1.0f);
    FIFOSampleBuffer_init(&(pRT->outputBuffer), pRT->numChannels);
}



void RateTransposer_deInit(RateTransposer *pRT)
{
    FIFOSampleBuffer_deInit(&(pRT->outputBuffer));
}

FIFOSampleBuffer *RateTransposer_getOutput(RateTransposer *pRT)
{
    return &(pRT->outputBuffer);
}

// Adds 'nSamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void RateTransposer_putSamples(RateTransposer *pRT, const SAMPLETYPE *samples, uint nSamples)
{
    RateTransposer_processSamples(pRT, samples, nSamples);
}

// Transposes sample rate by applying anti-alias filter to prevent folding.
// Returns amount of samples returned in the "dest" buffer.
// The maximum amount of samples that can be returned at a time is set by
// the 'set_returnBuffer_size' function.
void RateTransposer_processSamples(RateTransposer *pRT, const SAMPLETYPE *src, uint nSamples)
{
    uint count;
    uint sizeReq;

    if (nSamples == 0) return;

    sizeReq = (uint)((float)nSamples / pRT->fRate + 1.0f);
    count = RateTransposer_transpose(pRT, FIFOSampleBuffer_ptrEnd(&(pRT->outputBuffer), sizeReq), src, nSamples);
    FIFOSampleBuffer_putSamples(&(pRT->outputBuffer), count);
    return;
}

// Transposes the sample rate of the given samples using linear interpolation.
// Returns the number of samples returned in the "dest" buffer
uint RateTransposer_transpose(RateTransposer *pRT, SAMPLETYPE *dest, const SAMPLETYPE *src, uint nSamples)
{
    if (pRT->numChannels == 2)
    {
        return RateTransposer_transposeStereo(pRT, dest, src, nSamples);
    }
    else
    {
        return RateTransposer_transposeMono(pRT, dest, src, nSamples);
    }
}

// Sets the number of channels, 1 = mono, 2 = stereo
void RateTransposer_setChannels(RateTransposer *pRT, int nChannels)
{
    if (pRT->numChannels == nChannels) return;

    pRT->numChannels = nChannels;

    FIFOSampleBuffer_setChannels(&(pRT->outputBuffer), pRT->numChannels);

    // Inits the linear interpolation registers
    RateTransposer_resetRegisters(pRT);
}

void RateTransposer_moveSamples(RateTransposer *pRT, FIFOSampleBuffer *other) ///< Other pipe instance where from the receive the data.
{
    int oNumSamples = FIFOSampleBuffer_numSamples(other);//other.numSamples();
    RateTransposer_putSamples(pRT, FIFOSampleBuffer_ptrBegin(other), oNumSamples);
    FIFOSampleBuffer_receiveSamples(other, oNumSamples);
}

// Clears all the samples in the object
void RateTransposer_clear(RateTransposer *pRT)
{
    FIFOSampleBuffer_clear(&(pRT->outputBuffer));
}


//////////////////////////////////////////////////////////////////////////////
//
// RateTransposerInteger - integer arithmetic implementation
//

/// fixed-point interpolation routine precision
#define SCALE    65536




void RateTransposer_resetRegisters(RateTransposer *pRT)
{
    //pRT->iSlopeCount = 0;
    pRT->iSlopeCount = SCALE; //aaron.sun
    pRT->sPrevSampleL =
        pRT->sPrevSampleR = 0;
}



// Transposes the sample rate of the given samples using linear interpolation.
// 'Mono' version of the routine. Returns the number of samples returned in
// the "dest" buffer
uint RateTransposer_transposeMono(RateTransposer *pRT, SAMPLETYPE *dest, const SAMPLETYPE *src, uint nSamples)
{
    unsigned int i, used;
    LONG_SAMPLETYPE temp, vol1;

    if (nSamples == 0) return 0;  // no samples, no work

    used = 0;
    i = 0;

    // Process the last sample saved from the previous call first...
    while (pRT->iSlopeCount <= SCALE)
    {
        vol1 = (LONG_SAMPLETYPE)(SCALE - pRT->iSlopeCount);
        temp = vol1 * pRT->sPrevSampleL + pRT->iSlopeCount * src[0];
        dest[i] = (SAMPLETYPE)(temp / SCALE);
        i++;
        pRT->iSlopeCount += pRT->iRate;
    }
    // now always (iSlopeCount > SCALE)
    pRT->iSlopeCount -= SCALE;

    while (1)
    {
        while (pRT->iSlopeCount > SCALE)
        {
            pRT->iSlopeCount -= SCALE;
            used ++;
            if (used >= nSamples - 1) goto end;
        }
        vol1 = (LONG_SAMPLETYPE)(SCALE - pRT->iSlopeCount);
        temp = src[used] * vol1 + pRT->iSlopeCount * src[used + 1];
        dest[i] = (SAMPLETYPE)(temp / SCALE);

        i++;
        pRT->iSlopeCount += pRT->iRate;
    }
end:
    // Store the last sample for the next round
    pRT->sPrevSampleL = src[nSamples - 1];

    return i;
}


// Transposes the sample rate of the given samples using linear interpolation.
// 'Stereo' version of the routine. Returns the number of samples returned in
// the "dest" buffer
uint RateTransposer_transposeStereo(RateTransposer *pRT, SAMPLETYPE *dest, const SAMPLETYPE *src, uint nSamples)
{
    unsigned int srcPos, i, used;
    LONG_SAMPLETYPE temp, vol1;

    if (nSamples == 0) return 0;  // no samples, no work

    used = 0;
    i = 0;

    // Process the last sample saved from the sPrevSampleLious call first...
    while (pRT->iSlopeCount <= SCALE)
    {
        vol1 = (LONG_SAMPLETYPE)(SCALE - pRT->iSlopeCount);
        temp = vol1 * pRT->sPrevSampleL + pRT->iSlopeCount * src[0];
        dest[2 * i] = (SAMPLETYPE)(temp / SCALE);
        temp = vol1 * pRT->sPrevSampleR + pRT->iSlopeCount * src[1];
        dest[2 * i + 1] = (SAMPLETYPE)(temp / SCALE);
        i++;
        pRT->iSlopeCount += pRT->iRate;
    }
    // now always (iSlopeCount > SCALE)
    pRT->iSlopeCount -= SCALE;

    while (1)
    {
        while (pRT->iSlopeCount > SCALE)
        {
            pRT->iSlopeCount -= SCALE;
            used ++;
            if (used >= nSamples - 1) goto end;
        }
        srcPos = 2 * used;
        vol1 = (LONG_SAMPLETYPE)(SCALE - pRT->iSlopeCount);
        temp = src[srcPos] * vol1 + pRT->iSlopeCount * src[srcPos + 2];
        dest[2 * i] = (SAMPLETYPE)(temp / SCALE);
        temp = src[srcPos + 1] * vol1 + pRT->iSlopeCount * src[srcPos + 3];
        dest[2 * i + 1] = (SAMPLETYPE)(temp / SCALE);

        i++;
        pRT->iSlopeCount += pRT->iRate;
    }
end:
    // Store the last sample for the next round
    pRT->sPrevSampleL = src[2 * nSamples - 2];
    pRT->sPrevSampleR = src[2 * nSamples - 1];

    return i;
}


// Sets new target iRate. Normal iRate = 1.0, smaller values represent slower
// iRate, larger faster iRates.
void RateTransposer_setRate(RateTransposer *pRT, float newRate)
{
    pRT->iRate = (int)(newRate * SCALE + 0.5f);
    pRT->fRate = newRate;
}

