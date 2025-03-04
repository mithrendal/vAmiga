// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "MuxerTypes.h"

#include "SubComponent.h"
#include "AudioStream.h"
#include "AudioFilter.h"
#include "Chrono.h"
#include "Sampler.h"

namespace vamiga {

/* Architecture of the audio pipeline
 *
 *           Mux class
 *           -----------------------------------------------------
 *  State   |   ---------                                         |
 * machine -|->| Sampler |-> vol ->|                              |
 *    0     |   ---------          |                              |
 *          |                      |                              |
 *  State   |   ---------          |                              |
 * machine -|->| Sampler |-> vol ->|                              |
 *    1     |   ---------          |     pan     --------------   |
 *          |                      |--> l vol ->| Audio Stream |--|-> GUI
 *  State   |   ---------          |    r vol    --------------   |
 * machine -|->| Sampler |-> vol ->|                              |
 *    2     |   ---------          |                              |
 *          |                      |                              |
 *  State   |   ---------          |                              |
 * machine -|->| Sampler |-> vol ->|                              |
 *    3     |   ---------                                         |
 *           -----------------------------------------------------
 */

class Muxer : public SubComponent {

    friend class Paula;
    
    // Current configuration
    MuxerConfig config = {};
    
    // Underflow and overflow counters
    MuxerStats stats = {};
    
    // Master clock cycles per audio sample
    double cyclesPerSample = 0.0;

    // Fraction of a sample that hadn't been generated in synthesize
    double fraction = 0.0;

    // Time stamp of the last write pointer alignment
    util::Time lastAlignment;

    // Volume control
    Volume volume;

    // Volume scaling factors
    float vol[4];
    float volL;
    float volR;

    // Panning factors
    float pan[4];
    
    
    //
    // Sub components
    //
    
public:

    // Inputs (one Sampler for each of the four channels)
    Sampler sampler[4] = {
        
        Sampler(),
        Sampler(),
        Sampler(),
        Sampler()
    };

    // Output
    AudioStream<SAMPLE_T> stream;
    
    // Audio filters
    AudioFilter filterL = AudioFilter(amiga);
    AudioFilter filterR = AudioFilter(amiga);

    
    //
    // Initializing
    //
    
public:
    
    Muxer(Amiga& ref);

    // Resets the output buffer and the two audio filters
    void clear();


    //
    // Methods from AmigaObject
    //
    
private:
    
    const char *getDescription() const override { return "Muxer"; }
    void _dump(Category category, std::ostream& os) const override;
    
    
    //
    // Methods from AmigaComponent
    //
    
private:
    
    void _reset(bool hard) override;
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
        worker
        
        << config.samplingMethod
        << config.pan
        << config.vol
        << config.volL
        << config.volR
        << pan
        << vol
        << volL
        << volR;
    }

    template <class T>
    void applyToResetItems(T& worker, bool hard = true)
    {
        
    }

    isize _size() override { COMPUTE_SNAPSHOT_SIZE }
    u64 _checksum() override { COMPUTE_SNAPSHOT_CHECKSUM }
    isize _load(const u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    isize _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    isize didLoadFromBuffer(const u8 *buffer) override;
    
    
    //
    // Configuring
    //
    
public:
    
    const MuxerConfig &getConfig() const { return config; }
    void resetConfig() override;
    
    i64 getConfigItem(Option option) const;
    i64 getConfigItem(Option option, long id) const;
    void setConfigItem(Option option, i64 value);
    void setConfigItem(Option option, long id, i64 value);

    // double getSampleRate() const { return sampleRate; }
    void setSampleRate(double hz);

    // Needs to be called when the sampling rate or the CPU speed changes
    void adjustSpeed();


    //
    // Analyzing
    //
    
public:
    
    // Returns information about the gathered statistical information
    const MuxerStats &getStats() const { return stats; }

    // Returns true if the output volume is zero
    bool isMuted() const { return config.volL == 0 && config.volR == 0; }


    //
    // Controlling volume
    //
    
public:

    /* Starts to ramp up the volume. This function configures variables volume
     * and targetVolume to simulate a smooth audio fade in.
     */
    void rampUp();
    void rampUpFromZero();
    
    /* Starts to ramp down the volume. This function configures variables
     * volume and targetVolume to simulate a quick audio fade out.
     */
    void rampDown();
    
    
    //
    // Generating audio streams
    //
    
public:
    
    void synthesize(Cycle clock, Cycle target, long count);
    void synthesize(Cycle clock, Cycle target);

private:

    template <SamplingMethod method>
    void synthesize(Cycle clock, long count, double cyclesPerSample);
    
    // Handles a buffer underflow or overflow condition
    void handleBufferUnderflow();
    void handleBufferOverflow();
    
public:
    
    // Signals to ignore the next underflow or overflow condition
    void ignoreNextUnderOrOverflow();


    //
    // Reading audio samples
    //
    
public:
    
    // Copies a certain amout of audio samples into a buffer
    void copy(void *buffer, isize n);
    void copy(void *buffer1, void *buffer2, isize n);
    
    /* Returns a pointer to a buffer holding a certain amount of audio samples
     * without copying data. This function has been implemented for speedup.
     * Instead of copying ring buffer data into the target buffer, it returns
     * a pointer into the ringbuffer itself. The caller has to make sure that
     * the ring buffer's read pointer is not closer than n elements to the
     * buffer end.
     */
    SAMPLE_T *nocopy(isize n);
};

}
