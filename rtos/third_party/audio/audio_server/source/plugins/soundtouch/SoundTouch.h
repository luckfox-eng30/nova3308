//////////////////////////////////////////////////////////////////////////////
///
/// SoundTouch - main class for tempo/pitch/rate adjusting routines.
///
/// Notes:
/// - Initialize the SoundTouch object instance by setting up the sound stream
///   parameters with functions 'setSampleRate' and 'setChannels', then set
///   desired tempo/pitch/rate settings with the corresponding functions.
///
/// - The SoundTouch class behaves like a first-in-first-out pipeline: The
///   samples that are to be processed are fed into one of the pipe by calling
///   function 'putSamples', while the ready processed samples can be read
///   from the other end of the pipeline with function 'receiveSamples'.
///
/// - The SoundTouch processing classes require certain sized 'batches' of
///   samples in order to process the sound. For this reason the classes buffer
///   incoming samples until there are enough of samples available for
///   processing, then they carry out the processing step and consequently
///   make the processed samples available for outputting.
///
/// - For the above reason, the processing routines introduce a certain
///   'latency' between the input and output, so that the samples input to
///   SoundTouch may not be immediately available in the output, and neither
///   the amount of outputtable samples may not immediately be in direct
///   relationship with the amount of previously input samples.
///
/// - The tempo/pitch/rate control parameters can be altered during processing.
///   Please notice though that they aren't currently protected by semaphores,
///   so in multi-thread application external semaphore protection may be
///   required.
///
/// - This class utilizes classes 'TDStretch' for tempo change (without modifying
///   pitch) and 'RateTransposer' for changing the playback rate (that is, both
///   tempo and pitch in the same ratio) of the sound. The third available control
///   'pitch' (change pitch but maintain tempo) is produced by a combination of
///   combining the two other controls.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-04-04 22:47:28 +0300 (Wed, 04 Apr 2012) $
// File revision : $Revision: 4 $
//
// $Id: SoundTouch.h 141 2012-04-04 19:47:28Z oparviai $
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

#ifndef SoundTouch_H
#define SoundTouch_H

#include "AudioConfig.h"
#include "FIFOSampleBuffer.h"
#include "RateTransposer.h"
#include "TDStretch.h"
#include "STTypes.h"


/// Soundtouch library version string
#define SOUNDTOUCH_VERSION          "1.7.0"

/// SoundTouch library version id
#define SOUNDTOUCH_VERSION_ID       (10700)

//
// Available setting IDs for the 'setSetting' & 'get_setting' functions:

/// Enable/disable anti-alias filter in pitch transposer (0 = disable)
#define SETTING_USE_AA_FILTER       0

/// Pitch transposer anti-alias filter length (8 .. 128 taps, default = 32)
#define SETTING_AA_FILTER_LENGTH    1

/// Enable/disable quick seeking algorithm in tempo changer routine
/// (enabling quick seeking lowers CPU utilization but causes a minor sound
///  quality compromising)
#define SETTING_USE_QUICKSEEK       2

/// Time-stretch algorithm single processing sequence length in milliseconds. This determines
/// to how long sequences the original sound is chopped in the time-stretch algorithm.
/// See "STTypes.h" or README for more information.
#define SETTING_SEQUENCE_MS         3

/// Time-stretch algorithm seeking window length in milliseconds for algorithm that finds the
/// best possible overlapping location. This determines from how wide window the algorithm
/// may look for an optimal joining location when mixing the sound sequences back together.
/// See "STTypes.h" or README for more information.
#define SETTING_SEEKWINDOW_MS       4

/// Time-stretch algorithm overlap length in milliseconds. When the chopped sound sequences
/// are mixed back together, to form a continuous sound stream, this parameter defines over
/// how long period the two consecutive sequences are let to overlap each other.
/// See "STTypes.h" or README for more information.
#define SETTING_OVERLAP_MS          5


/// Call "getSetting" with this ID to query nominal average processing sequence
/// size in samples. This value tells approcimate value how many input samples
/// SoundTouch needs to gather before it does DSP processing run for the sample batch.
///
/// Notices:
/// - This is read-only parameter, i.e. setSetting ignores this parameter
/// - Returned value is approximate average value, exact processing batch
///   size may wary from time to time
/// - This parameter value is not constant but may change depending on
///   tempo/pitch/rate/samplerate settings.
#define SETTING_NOMINAL_INPUT_SEQUENCE        6


/// Call "getSetting" with this ID to query nominal average processing output
/// size in samples. This value tells approcimate value how many output samples
/// SoundTouch outputs once it does DSP processing run for a batch of input samples.
///
/// Notices:
/// - This is read-only parameter, i.e. setSetting ignores this parameter
/// - Returned value is approximate average value, exact processing batch
///   size may wary from time to time
/// - This parameter value is not constant but may change depending on
///   tempo/pitch/rate/samplerate settings.
#define SETTING_NOMINAL_OUTPUT_SEQUENCE        7

#define SOUNDTOUCH_FILE_PATH_A  "A:\\st.wav"
#define SOUNDTOUCH_FILE_PATH_D  "D:\\st.wav"
#define SOUNDTOUCH_MAX_FILE_SIZE    300*1024    //300kB  3~4s
#define SOUNDTOUCH_ABSOLUTE_VALUE   0
#define SOUNDTOUCH_NEED_FLUSH   0

typedef struct  /*: public FIFOProcessor*/
{
    /// Rate transposer class instance
    RateTransposer rateTransposer;

    /// Time-stretch class instance
    TDStretch tdStretch;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    float virtualRate;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    float virtualTempo;

    /// Virtual pitch parameter. Effective rate & tempo are calculated from these parameters.
    float virtualPitch;

    /// Flag: Has sample rate been set?
    int  bSrateSet;

    /// Output sample buffer point
    FIFOSampleBuffer *output;
    /// Number of channels
    uint  channels;

    /// Effective 'rate' value calculated from 'virtualRate', 'virtualTempo' and 'virtualPitch'
    float rate;

    /// Effective 'tempo' value calculated from 'virtualRate', 'virtualTempo' and 'virtualPitch'
    float tempo;
} SoundTouch;

extern SoundTouch *mSoundTouch;

int soundtouch_record_init();
int soundtouch_record_deinit();

void voice_change_run(player_handle_t player, char *pfile_name);
int voice_change_init(uint samplerate, uint nChannls, float Pitch, float Rate, float Tempo);

void SoundTouch_init(SoundTouch *pST);
void SoundTouch_deInit(SoundTouch *pST);

/// Sets new rate control value. Normal rate = 1.0, smaller values
/// represent slower rate, larger faster rates.
void SoundTouch_setRate(SoundTouch *pST, float newRate);

/// Sets new tempo control value. Normal tempo = 1.0, smaller values
/// represent slower tempo, larger faster tempo.
void SoundTouch_setTempo(SoundTouch *pST, float newTempo);

/// Sets new rate control value as a difference in percents compared
/// to the original rate (-50 .. +100 %)
void SoundTouch_setRateChange(SoundTouch *pST, float newRate);

/// Sets new tempo control value as a difference in percents compared
/// to the original tempo (-50 .. +100 %)
void SoundTouch_setTempoChange(SoundTouch *pST, float newTempo);

/// Sets new pitch control value. Original pitch = 1.0, smaller values
/// represent lower pitches, larger values higher pitch.
void SoundTouch_setPitch(SoundTouch *pST, float newPitch);

/// Sets pitch change in octaves compared to the original pitch
/// (-1.00 .. +1.00)
void SoundTouch_setPitchOctaves(SoundTouch *pST, float newPitch);

/// Sets pitch change in semi-tones compared to the original pitch
/// (-12 .. +12)
void SoundTouch_setPitchSemiTones(SoundTouch *pST, int newPitch);
void SoundTouch_setPitchSemiTones1(SoundTouch *pST, float newPitch);

/// Sets the number of channels, 1 = mono, 2 = stereo
void SoundTouch_setChannels(SoundTouch *pST, uint numChannels);

/// Sets sample rate.
void SoundTouch_setSampleRate(SoundTouch *pST, uint srate);

/// Flushes the last samples from the processing pipeline to the output.
/// Clears also the internal processing buffers.
//
/// Note: This function is meant for extracting the last samples of a sound
/// stream. This function may introduce additional blank samples in the end
/// of the sound stream, and thus it's not recommended to call this function
/// in the middle of a sound stream.
void SoundTouch_flush(SoundTouch *pST);

/// Adds 'numSamples' pcs of samples from the 'samples' memory position into
/// the input of the object. Notice that sample rate _has_to_ be set before
/// calling this function, otherwise throws a runtime_error exception.
void SoundTouch_putSamples(SoundTouch *pST,
                           const SAMPLETYPE *samples,  ///< Pointer to sample buffer.
                           uint numSamples                         ///< Number of samples in buffer. Notice
                           ///< that in case of stereo-sound a single sample
                           ///< contains data for both channels.
                          );
// Output samples from beginning of the sample buffer. Copies demanded number
// of samples to output and removes them from the sample buffer. If there
// are less than 'numsample' samples in the buffer, returns all available.
//
// Returns number of samples copied.
uint SoundTouch_receiveSamples(SoundTouch *pST, SAMPLETYPE *output1, uint maxSamples);

/// Clears all the samples in the object's output and internal processing
/// buffers.
void SoundTouch_clear(SoundTouch *pST);

/// Changes a setting controlling the processing system behaviour. See the
/// 'SETTING_...' defines for available setting ID's.
///
/// \return 'TRUE' if the setting was succesfully changed
int SoundTouch_setSetting(SoundTouch *pST,
                          int settingId,   ///< Setting ID number. see SETTING_... defines.
                          int value        ///< New setting value.
                         );

/// Reads a setting controlling the processing system behaviour. See the
/// 'SETTING_...' defines for available setting ID's.
///
/// \return the setting value.
int SoundTouch_getSetting(SoundTouch *pST,
                          int settingId    ///< Setting ID number, see SETTING_... defines.
                         );

/// Returns number of samples currently unprocessed.
uint SoundTouch_numUnprocessedSamples(SoundTouch *pST);



#endif
