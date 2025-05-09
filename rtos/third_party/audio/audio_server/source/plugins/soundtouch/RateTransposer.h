////////////////////////////////////////////////////////////////////////////////
///
/// Sample rate transposer. Changes sample rate by using linear interpolation
/// together with anti-alias filtering (first order interpolation with anti-
/// alias filtering should be quite adequate for this application).
///
/// Use either of the derived classes of 'RateTransposerInteger' or
/// 'RateTransposerFloat' for corresponding integer/floating point tranposing
/// algorithm implementation.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2009-02-21 18:00:14 +0200 (Sat, 21 Feb 2009) $
// File revision : $Revision: 4 $
//
// $Id: RateTransposer.h 63 2009-02-21 16:00:14Z oparviai $
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

#ifndef RateTransposer_H
#define RateTransposer_H

#include <stddef.h>
#include "FIFOSampleBuffer.h"

#include "STTypes.h"

// A common linear samplerate transposer class.
///
/// Note: Use function "RateTransposer::newInstance()" to create a new class
/// instance instead of the "new" operator; that function automatically
/// chooses a correct implementation depending on if integer or floating
/// arithmetics are to be used.
typedef struct  /*:public FIFOProcessor*/
{
    int iSlopeCount;
    int iRate;
    SAMPLETYPE sPrevSampleL, sPrevSampleR;
    float fRate;

    int numChannels;

    /// Output sample buffer
    FIFOSampleBuffer outputBuffer;

} RateTransposer;

void RateTransposer_init(RateTransposer *pRT);
void RateTransposer_deInit(RateTransposer *pRT);

/// Returns the output buffer object
FIFOSampleBuffer *RateTransposer_getOutput(RateTransposer *pRT);

/// Sets new target rate. Normal rate = 1.0, smaller values represent slower
/// rate, larger faster rates.
void RateTransposer_setRate(RateTransposer *pRT, float newRate);

/// Sets the number of channels, 1 = mono, 2 = stereo
void RateTransposer_setChannels(RateTransposer *pRT, int channels);

/// Adds 'numSamples' pcs of samples from the 'samples' memory position into
/// the input of the object.
void RateTransposer_putSamples(RateTransposer *pRT, const SAMPLETYPE *samples, uint numSamples);

void RateTransposer_moveSamples(RateTransposer *pRT, FIFOSampleBuffer *other);///< Other pipe instance where from the receive the data.

/// Clears all the samples in the object
void RateTransposer_clear(RateTransposer *pRT);


#endif
