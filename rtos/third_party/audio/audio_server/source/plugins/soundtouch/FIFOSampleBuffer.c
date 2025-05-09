////////////////////////////////////////////////////////////////////////////////
///
/// A buffer class for temporarily storaging sound samples, operates as a
/// first-in-first-out pipe.
///
/// Samples are added to the end of the sample buffer with the 'putSamples'
/// function, and are received from the beginning of the buffer by calling
/// the 'receiveSamples' function. The class automatically removes the
/// outputted samples from the buffer, as well as grows the buffer size
/// whenever necessary.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-06-13 22:29:53 +0300 (Wed, 13 Jun 2012) $
// File revision : $Revision: 4 $
//
// $Id: FIFOSampleBuffer.cpp 143 2012-06-13 19:29:53Z oparviai $
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
#define NOT_INCLUDE_OTHER
#include <stdlib.h>
//#include <memory.h>
#include <string.h>

#include "FIFOSampleBuffer.h"

/// Rewind the buffer by moving data from position pointed by 'bufferPos' to real
/// beginning of the buffer.
void FIFOSampleBuffer_rewind(FIFOSampleBuffer *pFSB);

/// Ensures that the buffer has capacity for at least this many samples.
void FIFOSampleBuffer_ensureCapacity(FIFOSampleBuffer *pFSB, uint capacityRequirement);

/// Returns current capacity.
uint FIFOSampleBuffer_getCapacity(FIFOSampleBuffer *pFSB) ;

//aaron.sun
//#define FIFO_SIZE 4160 * 4
//SAMPLETYPE FIFOBuf1[FIFO_SIZE];
//SAMPLETYPE FIFOBuf2[FIFO_SIZE];
//SAMPLETYPE FIFOBuf3[FIFO_SIZE];
//uint FIFONUM = 0;

// Constructor
void FIFOSampleBuffer_init(FIFOSampleBuffer *pFSB, int numChannels)
{
    pFSB->sizeInBytes = 0; // reasonable initial value
    pFSB->buffer = NULL;
    pFSB->bufferUnaligned = NULL;
    pFSB->samplesInBuffer = 0;
    pFSB->bufferPos = 0;
    pFSB->channels = (uint)numChannels;
    //FIFONUM++; //aaron.sun
    //pFSB->bufnum = FIFONUM; //aaron.sun
    FIFOSampleBuffer_ensureCapacity(pFSB, 32);    // allocate initial capacity
}


// destructor
void FIFOSampleBuffer_deInit(FIFOSampleBuffer *pFSB)
{

    audio_free(pFSB->bufferUnaligned);//aaron.sun
    pFSB->bufferUnaligned = NULL;

    pFSB->buffer = NULL;
}


// Sets number of channels, 1 = mono, 2 = stereo
void FIFOSampleBuffer_setChannels(FIFOSampleBuffer *pFSB, int numChannels)
{
    uint usedBytes;

    usedBytes = pFSB->channels * pFSB->samplesInBuffer;
    pFSB->channels = (uint)numChannels;
    pFSB->samplesInBuffer = usedBytes / pFSB->channels;
}


// if output location pointer 'bufferPos' isn't zero, 'rewinds' the buffer and
// zeroes this pointer by copying samples from the 'bufferPos' pointer
// location on to the beginning of the buffer.
void FIFOSampleBuffer_rewind(FIFOSampleBuffer *pFSB)
{
    if (pFSB->buffer && pFSB->bufferPos)
    {
        memmove(pFSB->buffer, FIFOSampleBuffer_ptrBegin(pFSB), sizeof(SAMPLETYPE) * pFSB->channels * pFSB->samplesInBuffer);
        pFSB->bufferPos = 0;
    }
}


// Adds 'numSamples' pcs of samples from the 'samples' memory position to
// the sample buffer.
void FIFOSampleBuffer_putSamples2(FIFOSampleBuffer *pFSB, const SAMPLETYPE *samples, uint nSamples)
{
    memcpy(FIFOSampleBuffer_ptrEnd(pFSB, nSamples), samples, sizeof(SAMPLETYPE) * nSamples * pFSB->channels);
    pFSB->samplesInBuffer += nSamples;
}


// Increases the number of samples in the buffer without copying any actual
// samples.
//
// This function is used to update the number of samples in the sample buffer
// when accessing the buffer directly with 'ptrEnd' function. Please be
// careful though!
void FIFOSampleBuffer_putSamples(FIFOSampleBuffer *pFSB, uint nSamples)
{
    uint req;

    req = pFSB->samplesInBuffer + nSamples;
    FIFOSampleBuffer_ensureCapacity(pFSB, req);
    pFSB->samplesInBuffer += nSamples;
}


// Returns a pointer to the end of the used part of the sample buffer (i.e.
// where the new samples are to be inserted). This function may be used for
// inserting new samples into the sample buffer directly. Please be careful!
//
// Parameter 'slackCapacity' tells the function how much free capacity (in
// terms of samples) there _at least_ should be, in order to the caller to
// succesfully insert all the required samples to the buffer. When necessary,
// the function grows the buffer size to comply with this requirement.
//
// When using this function as means for inserting new samples, also remember
// to increase the sample count afterwards, by calling  the
// 'putSamples(numSamples)' function.
SAMPLETYPE *FIFOSampleBuffer_ptrEnd(FIFOSampleBuffer *pFSB, uint slackCapacity)
{
    FIFOSampleBuffer_ensureCapacity(pFSB, pFSB->samplesInBuffer + slackCapacity);
    //printf("pFSB->samplesInBuffer = %d", pFSB->samplesInBuffer);
    return pFSB->buffer + pFSB->samplesInBuffer * pFSB->channels;
}


// Returns a pointer to the beginning of the currently non-outputted samples.
// This function is provided for accessing the output samples directly.
// Please be careful!
//
// When using this function to output samples, also remember to 'remove' the
// outputted samples from the buffer by calling the
// 'receiveSamples(numSamples)' function
SAMPLETYPE *FIFOSampleBuffer_ptrBegin(FIFOSampleBuffer *pFSB)
{

    return pFSB->buffer + pFSB->bufferPos * pFSB->channels;
}


// Ensures that the buffer has enought capacity, i.e. space for _at least_
// 'capacityRequirement' number of samples. The buffer is grown in steps of
// 4 kilobytes to eliminate the need for frequently growing up the buffer,
// as well as to round the buffer size up to the virtual memory page size.
void FIFOSampleBuffer_ensureCapacity(FIFOSampleBuffer *pFSB, uint capacityRequirement)
{
    SAMPLETYPE *tempUnaligned = NULL, *temp;

    if (capacityRequirement > FIFOSampleBuffer_getCapacity(pFSB))
    {
        // enlarge the buffer in 4kbyte steps (round up to next 4k boundary)
        pFSB->sizeInBytes = (capacityRequirement * pFSB->channels * sizeof(SAMPLETYPE) + 4095) & (uint) - 4096;

        /*
        if((pFSB->sizeInBytes  + 16) <= FIFO_SIZE*2)
        {
            if(pFSB->bufnum==1)
                tempUnaligned = (SAMPLETYPE*)FIFOBuf1;
            else if(pFSB->bufnum==2)
                tempUnaligned = (SAMPLETYPE*)FIFOBuf2;
            else
                tempUnaligned = (SAMPLETYPE*)FIFOBuf3;

            memset(tempUnaligned,0,FIFO_SIZE*2);

            pFSB->sizeInBytes = FIFO_SIZE*2;
        }
        */

        if (pFSB->bufferUnaligned != NULL)
        {
            audio_free(pFSB->bufferUnaligned);
        }

        tempUnaligned = (SAMPLETYPE *)audio_malloc(pFSB->sizeInBytes  + 16);
        if (tempUnaligned == NULL)
        {
            RK_AUDIO_LOG_E("Couldn't allocate memory! %d\n", pFSB->sizeInBytes + 16);
        }

        // Align the buffer to begin at 16byte cache line boundary for optimal performance
        temp = (SAMPLETYPE *)(((ulong)tempUnaligned + 15) & (ulong) - 16);
        if (pFSB->samplesInBuffer)
        {
            memcpy(temp, FIFOSampleBuffer_ptrBegin(pFSB), pFSB->samplesInBuffer * pFSB->channels * sizeof(SAMPLETYPE));
        }

        pFSB->buffer = temp;
        pFSB->bufferUnaligned = tempUnaligned;
        pFSB->bufferPos = 0;
    }
    else
    {
        // simply rewind the buffer (if necessary)
        FIFOSampleBuffer_rewind(pFSB);
    }
}


// Returns the current buffer capacity in terms of samples
uint FIFOSampleBuffer_getCapacity(FIFOSampleBuffer *pFSB)
{
    return pFSB->sizeInBytes / (pFSB->channels * sizeof(SAMPLETYPE));
}


// Returns the number of samples currently in the buffer
uint FIFOSampleBuffer_numSamples(FIFOSampleBuffer *pFSB)
{
    return pFSB->samplesInBuffer;
}


// Output samples from beginning of the sample buffer. Copies demanded number
// of samples to output and removes them from the sample buffer. If there
// are less than 'numsample' samples in the buffer, returns all available.
//
// Returns number of samples copied.
uint FIFOSampleBuffer_receiveSamples2(FIFOSampleBuffer *pFSB, SAMPLETYPE *output, uint maxSamples)
{
    uint num;

    num = (maxSamples > pFSB->samplesInBuffer) ? pFSB->samplesInBuffer : maxSamples;

    memcpy(output, FIFOSampleBuffer_ptrBegin(pFSB), pFSB->channels * sizeof(SAMPLETYPE) * num);
    return FIFOSampleBuffer_receiveSamples(pFSB, num);
}


// Removes samples from the beginning of the sample buffer without copying them
// anywhere. Used to reduce the number of samples in the buffer, when accessing
// the sample buffer with the 'ptrBegin' function.
uint FIFOSampleBuffer_receiveSamples(FIFOSampleBuffer *pFSB, uint maxSamples)
{
    if (maxSamples >= pFSB->samplesInBuffer)
    {
        uint temp;

        temp = pFSB->samplesInBuffer;
        pFSB->samplesInBuffer = 0;
        return temp;
    }

    pFSB->samplesInBuffer -= maxSamples;
    pFSB->bufferPos += maxSamples;

    return maxSamples;
}


// Returns nonzero if the sample buffer is empty
int FIFOSampleBuffer_isEmpty(FIFOSampleBuffer *pFSB)
{
    return (pFSB->samplesInBuffer == 0) ? 1 : 0;
}


// Clears the sample buffer
void FIFOSampleBuffer_clear(FIFOSampleBuffer *pFSB)
{
    pFSB->samplesInBuffer = 0;
    pFSB->bufferPos = 0;
}


/// allow trimming (downwards) amount of samples in pipeline.
/// Returns adjusted amount of samples
uint FIFOSampleBuffer_adjustAmountOfSamples(FIFOSampleBuffer *pFSB, uint numSamples)
{
    if (numSamples < pFSB->samplesInBuffer)
    {
        pFSB->samplesInBuffer = numSamples;
    }
    return pFSB->samplesInBuffer;
}

