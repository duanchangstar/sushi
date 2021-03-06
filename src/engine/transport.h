/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Main entry point to Sushi
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

 /**
 * @Brief Handles time, tempo and start/stop inside the engine
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_TRANSPORT_H
#define SUSHI_TRANSPORT_H

#include <library/constants.h>
#include "library/time.h"
#include "library/types.h"
#include "library/rt_event.h"

namespace sushi {
namespace engine {

constexpr float DEFAULT_TEMPO = 120;
/* Ableton recomends this to be around 10 Hz */
constexpr int LINK_UPDATE_RATE = 48000 / (10 * AUDIO_CHUNK_SIZE);

class Transport
{
public:
    explicit Transport(float sample_rate) : _samplerate(sample_rate) {}
    ~Transport() {}

    /**
     * @brief Set the current time. Called from the audio thread
     * @param timestamp The current timestamp
     * @param samples   The number of samples passed
     */
    void set_time(Time timestamp, int64_t samples);

    /**
     * @brief Set the output latency, i.e. the time it takes for the audio to travel through
     *        the driver stack to a physical output, including any DAC latency. Should be
     *        called by the audio frontend
     * @param output_latency The output latency
     */
    void set_latency(Time output_latency)
    {
        _latency = output_latency;
    }

    /**
     * @brief Set the time signature used in the engine
     * @param signature The new time signature to use
     */
    void set_time_signature(TimeSignature signature);

    /**
     * @brief Set the tempo of the engine in beats (quarter notes) per minute
     * @param tempo The new tempo in beats per minute
     */
    void set_tempo(float tempo) {_tempo = tempo;}

    /**
     * @brief Return the current set playing mode
     * @return the current set playing mode
     */
    PlayingMode playing_mode() const {return _mode;}

    /**
     * @brief Set the playing mode, i.e. playing, stopped, recording etc..
     * @param mode The new playing mode.
     */
    void set_playing_mode(PlayingMode mode) {_mode = mode;}

    /**
     * @brief Return the current current mode of synchronising tempo and beats
     * @return the current set mode of synchronisation
     */
    SyncMode sync_mode() const {return _sync_mode;}

    /**
     * @brief Set the current mode of synchronising tempo and beats
     * @param mode The mode of synchronisation to use
     */
    void set_sync_mode(SyncMode mode) {_sync_mode = mode;}

    /**
     * @return Set the sample rate.
     * @param sample_rate The current sample rate the engine is running at
     */
    void set_sample_rate(float sample_rate) {_samplerate = sample_rate;}

    /**
     * @brief Query the current time, Safe to call from rt and non-rt context. If called from
     *        an rt context it will return the time at which sample 0 of the current audio
     *        chunk being processed will appear on an output.
     * @return The current processing time.
     */
    Time current_process_time() const {return _time;}

    /**
     * @brief Query the current samplecount. Safe to call from rt and non-rt context. If called
     *        from an rt context it will return the total number of samples passed before sample
     *        0 of the current audio chunk being processed.
     * @return Total samplecount
     */
    int64_t current_samples() const {return _sample_count;}

    /**
     * @brief If the transport is currently playing or not. If in master mode, this always
     *        returns true as we don't really have a concept of stop and start yet and
     *        as sushi as of now is mostly intended for live use.
     * @return true if the transport is currently playing, false if stopped
     */
    bool playing() const {return _mode != PlayingMode::STOPPED;}

    /**
     * @brief Query the current time signature being used
     * @return A TimeSignature struct describing the current time signature
     */
    TimeSignature current_time_signature() const {return _time_signature;}

    /**
     * @brief Query the current tempo. Safe to call from rt and non-rt context but will
     *        only return approximate value if called from a non-rt context
     * @return A float representing the tempo in beats per minute
     */
    float current_tempo() const {return _tempo;}

    /**
    * @brief Query the position in beats (quarter notes) in the current bar with an optional
    *        sample offset from the start of the current processing chunk.
    *        The number of quarter notes in a bar will depend on the time signature set.
    *        For 4/4 time this will return a float in the range (0, 4), for 6/8 time this
    *        will return a range (0, 2). Safe to call from rt and non-rt context but will
    *        only return approximate values if called from a non-rt context
    * @param samples The number of samples offset form sample 0 in the current audio chunk
    * @return A double representing the position in the current bar.
    */
    double current_bar_beats(int samples) const;
    double current_bar_beats() const {return _current_bar_beat_count;}

    /**
     * @brief Query the current position in beats (quarter notes( with an optional sample
     *        offset from the start of the current processing chunk. This runs continuously
     *        and is monotonically increasing. Safe to call from rt and non-rt context but
     *        will only return approximate values if called from a non-rt context
     * @param samples The number of samples offset form sample 0 in the current audio chunk
     * @return A double representing the current position in quarter notes.
     */
    double current_beats(int samples) const;
    double current_beats() const {return _beat_count;}

    /**
     * @return Query the position, in beats (quarter notes), of the start of the current bar.
     *         Safe to call from rt and non-rt context but will only return approximate values
     *         if called from a non-rt context
     * @return A double representing the start position of the current bar in quarter notes
     */
    double current_bar_start_beats() const {return _bar_start_beat_count;}

private:
    void _update_internals();

    int64_t         _sample_count{0};
    Time            _time{0};
    Time            _latency{0};
    float           _tempo{DEFAULT_TEMPO};
    double          _current_bar_beat_count{0.0};
    double          _beat_count{0.0};
    double          _bar_start_beat_count{0};
    double          _beats_per_chunk{0};
    float           _beats_per_bar;
    float           _samplerate{};
    SyncMode        _sync_mode{SyncMode::INTERNAL};
    TimeSignature   _time_signature{4, 4};
    PlayingMode     _mode{PlayingMode::PLAYING};
};

} // namespace engine
} // namespace sushi

#endif //SUSHI_TRANSPORT_H
