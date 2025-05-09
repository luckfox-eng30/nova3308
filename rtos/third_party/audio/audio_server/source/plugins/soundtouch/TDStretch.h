////////////////////////////////////////////////////////////////////////////////
///
/// Sampled sound tempo changer/time stretch algorithm. Changes the sound tempo
/// while maintaining the original pitch by using a time domain WSOLA-like method
/// with several performance-increasing tweaks.
///
/// Note : MMX/SSE optimized functions reside in separate, platform-specific files
/// 'mmx_optimized.cpp' and 'sse_optimized.cpp'
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-04-01 22:49:30 +0300 (Sun, 01 Apr 2012) $
// File revision : $Revision: 4 $
//
// $Id: TDStretch.h 137 2012-04-01 19:49:30Z oparviai $
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

#ifndef TDStretch_H
#define TDStretch_H

#include <stddef.h>
#include "STTypes.h"
#include "RateTransposer.h"
#include "FIFOSampleBuffer.h"

/// Default values for sound processing parameters:
/// Notice that the default parameters are tuned for contemporary popular music
/// processing. For speech processing applications these parameters suit better:
///     #define DEFAULT_SEQUENCE_MS     40
///     #define DEFAULT_SEEKWINDOW_MS   15
///     #define DEFAULT_OVERLAP_MS      8
///

/// Default length of a single processing sequence, in milliseconds. This determines to how
/// long sequences the original sound is chopped in the time-stretch algorithm.
///
/// The larger this value is, the lesser sequences are used in processing. In principle
/// a bigger value sounds better when slowing down tempo, but worse when increasing tempo
/// and vice versa.
///
/// Increasing this value reduces computational burden & vice versa.
//#define DEFAULT_SEQUENCE_MS         40
#define DEFAULT_SEQUENCE_MS         USE_AUTO_SEQUENCE_LEN

/// Giving this value for the sequence length sets automatic parameter value
/// according to tempo setting (recommended)
#define USE_AUTO_SEQUENCE_LEN       0

/// Seeking window default length in milliseconds for algorithm that finds the best possible
/// overlapping location. This determines from how wide window the algorithm may look for an
/// optimal joining location when mixing the sound sequences back together.
///
/// The bigger this window setting is, the higher the possibility to find a better mixing
/// position will become, but at the same time large values may cause a "drifting" artifact
/// because consequent sequences will be taken at more uneven intervals.
///
/// If there's a disturbing artifact that sounds as if a constant frequency was drifting
/// around, try reducing this setting.
///
/// Increasing this value increases computational burden & vice versa.
//#define DEFAULT_SEEKWINDOW_MS       15
#define DEFAULT_SEEKWINDOW_MS       USE_AUTO_SEEKWINDOW_LEN

/// Giving this value for the seek window length sets automatic parameter value
/// according to tempo setting (recommended)
#define USE_AUTO_SEEKWINDOW_LEN     0

/// Overlap length in milliseconds. When the chopped sound sequences are mixed back together,
/// to form a continuous sound stream, this parameter defines over how long period the two
/// consecutive sequences are let to overlap each other.
///
/// This shouldn't be that critical parameter. If you reduce the DEFAULT_SEQUENCE_MS setting
/// by a large amount, you might wish to try a smaller value on this.
///
/// Increasing this value increases computational burden & vice versa.
#define DEFAULT_OVERLAP_MS      8


/// Class that does the time-stretch (tempo change) effect for the processed
/// sound.
typedef struct   /*: public FIFOProcessor*/
{
    int channels;
    int sampleReq;
    float tempo;

    SAMPLETYPE *pMidBuffer;
    SAMPLETYPE *pMidBufferUnaligned;
    int overlapLength;
    int seekLength;
    int seekWindowLength;
    int overlapDividerBits;
    int slopingDivider;
    float nominalSkip;
    float skipFract;
    FIFOSampleBuffer outputBuffer;
    FIFOSampleBuffer inputBuffer;

    int sampleRate;
    int sequenceMs;
    int seekWindowMs;
    int overlapMs;
    int bAutoSeqSetting;
    int bAutoSeekSetting;
} TDStretch;


void TDStretch_init(TDStretch *pTDS);
void TDStretch_deInit(TDStretch *pTDS);


/// Returns the output buffer object
FIFOSampleBuffer *TDStretch_getOutput(TDStretch *pTDS);

/// Returns the input buffer object
FIFOSampleBuffer *TDStretch_getInput(TDStretch *pTDS);

/// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower
/// tempo, larger faster tempo.
void TDStretch_setTempo(TDStretch *pTDS, float newTempo);

/// Returns nonzero if there aren't any samples available for outputting.
void TDStretch_clear(TDStretch *pTDS);

/// Clears the input buffer
void TDStretch_clearInput(TDStretch *pTDS);

/// Sets the number of channels, 1 = mono, 2 = stereo
void TDStretch_setChannels(TDStretch *pTDS, int numChannels);

/// Sets routine control parameters. These control are certain time constants
/// defining how the sound is stretched to the desired duration.
//
/// 'sampleRate' = sample rate of the sound
/// 'sequenceMS' = one processing sequence length in milliseconds
/// 'seekwindowMS' = seeking window length for scanning the best overlapping
///      position
/// 'overlapMS' = overlapping length
void TDStretch_setParameters(TDStretch *pTDS, int sampleRate,         ///< Samplerate of sound being processed (Hz)
                             int sequenceMS,      ///< Single processing sequence length (ms)
                             int seekwindowMS,    ///< Offset seeking window length (ms)
                             int overlapMS        ///< Sequence overlapping length (ms)
                            );

/// Get routine control parameters, see setParameters() function.
/// Any of the parameters to this function can be NULL, in such case corresponding parameter
/// value isn't returned.
void TDStretch_getParameters(TDStretch *pTDS, int *pSampleRate, int *pSequenceMs, int *pSeekWindowMs, int *pOverlapMs);

/// Adds 'numsamples' pcs of samples from the 'samples' memory position into
/// the input of the object.
void TDStretch_putSamples(
    TDStretch *pTDS,
    const SAMPLETYPE *samples,  ///< Input sample data
    uint numSamples                         ///< Number of samples in 'samples' so that one sample
    ///< contains both channels if stereo
);
void TDStretch_moveSamples(TDStretch *pTDS, FIFOSampleBuffer *other   ///< Other pipe instance where from the receive the data.
                          );

/// return nominal input sample requirement for triggering a processing batch
int TDStretch_getInputSampleReq(TDStretch *pTDS);


/// return nominal output sample amount when running a processing batch
int TDStretch_getOutputBatchSize(TDStretch *pTDS);



#endif  /// TDStretch_H
