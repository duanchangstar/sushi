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
 * @brief Real time audio processing engine
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_ENGINE_H
#define SUSHI_ENGINE_H

#include <memory>
#include <map>
#include <vector>
#include <utility>
#include <mutex>

#include "twine/twine.h"

#include "engine/event_dispatcher.h"
#include "engine/base_engine.h"
#include "track.h"
#include "engine/receiver.h"
#include "engine/transport.h"
#include "engine/host_control.h"
#include "engine/controller.h"
#include "library/time.h"
#include "library/sample_buffer.h"
#include "library/elk_allocator.h"
#include "library/internal_plugin.h"
#include "library/midi_decoder.h"
#include "library/rt_event_fifo.h"
#include "library/types.h"
#include "library/performance_timer.h"

namespace sushi {
namespace engine {

class ClipDetector
{
public:
    ClipDetector(float sample_rate)
    {
        this->set_sample_rate(sample_rate);
    }

    void set_sample_rate(float samplerate);

    void set_input_channels(int channels);

    void set_output_channels(int channels);
    /**
     * @brief Find clipped samples in a buffer and send notifications
     * @param buffer The audio buffer to process
     * @param queue Endpoint for clipping notifications
     * @param audio_input Set to true if the audio buffer comes directly from the an audio inout (i.e. before any processing)
     */
    void detect_clipped_samples(const ChunkSampleBuffer& buffer, RtSafeRtEventFifo& queue, bool audio_input);

private:

    unsigned int _interval;
    std::vector<unsigned int> _input_clip_count;
    std::vector<unsigned int> _output_clip_count;
};


constexpr int MAX_RT_PROCESSOR_ID = 1000;

class AudioEngine : public BaseEngine
{
public:
    SUSHI_DECLARE_NON_COPYABLE(AudioEngine);

    /**
     * @brief Construct a new AudioEngine
     * @param sample_rate The sample to use in Hz
     * @param rt_cpu_cores The maximum number of cpu cores to use for audio processing. Default
     *                     is 1 and means that audio processing is done only in the rt callback
     *                     of the audio frontend.
     *                     With values >1 tracks will be processed in parallel threads.
     */
    explicit AudioEngine(float sample_rate, int rt_cpu_cores = 1);

     ~AudioEngine();

    /**
     * @brief Configure the Engine with a new samplerate.
     * @param sample_rate The new sample rate in Hz
     */
    void set_sample_rate(float sample_rate) override;

    /**
     * @brief Set the number of input audio channels, set by the audio frontend before
     *        starting processing
     * @param channels The number of audio channels to use
     */
    void set_audio_input_channels(int channels) override;

    /**
     * @brief Set the number of output audio channels, set by the audio frontend before
     *        starting processing
     * @param channels The number of audio channels to use
     */
    void set_audio_output_channels(int channels) override;

    /**
     * @brief Set the number of control voltage inputs, set by the audio frontend before
     *        starting processing
     * @param channels The number of cv input channels to use
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus set_cv_input_channels(int channels) override;

    /**
     * @brief Set the number of control voltage outputs, set by the audio frontend before
     *        starting processing
     * @param channels The number of cv input channels to use
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus set_cv_output_channels(int channels) override;

    /**
     * @brief Connect an engine input channel to an input channel of a given track.
     *        Not safe to call while the engine is running.
     * @param input_channel Index of the engine input channel to connect.
     * @param track_channel Index of the input channel of the track to connect to.
     * @param track_name The unique name of the track.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_audio_input_channel(int input_channel,
                                                   int track_channel,
                                                   const std::string& track_name) override;

    /**
     * @brief Connect an output channel of a track to an engine output channel.
     *        Not safe to call while the engine is running.
     * @param output_channel Index of the engine output channel to connect to.
     * @param track_channel Index of the output channel of the track to connect from.
     * @param track_name The unique name of the track.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_audio_output_channel(int output_channel,
                                                    int track_channel,
                                                    const std::string& track_name) override;

    /**
     * @brief Connect a stereo pair (bus) from an engine input bus to an input bus of
     *        given track. Not safe to use while the engine in running.
     * @param input_bus The engine input bus to use.
     *        bus 0 refers to channels 0-1, 1 to channels 2-3, etc
     * @param track_bus The input bus of the track to connect to.
     * @param track_name The unique name of the track.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_audio_input_bus(int input_bus,
                                               int track_bus,
                                               const std::string& track_name) override;

    /**
     * @brief Connect an output bus of a track to an output bus (stereo pair)
     *        Not safe to use while the engine in running.
     * @param output_bus The engine outpus bus to use.
     *        bus 0 refers to channels 0-1, 1 to channels 2-3, etc
     * @param track_bus The output bus of the track to connect from.
     * @param track_name The unique name of the track.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_audio_output_bus(int output_bus,
                                                int track_bus,
                                                const std::string& track_name) override;

    /**
     * @brief Connect a control voltage input to control a parameter on a processor
     * @param processor_name The unique name of the processor.
     * @param parameter_name The name of the parameter to connect to
     * @param cv_input_id The Cv input port id to use
     * @return EngineReturnStatud::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_cv_to_parameter(const std::string& processor_name,
                                               const std::string& parameter_name,
                                               int cv_input_id) override;

    /**
     * @brief Connect a parameter on a processor to a control voltage output
     * @param processor_name The unique name of the processor.
     * @param parameter_name The name of the parameter to connect from
     * @param cv_output_id The Cv output port id to use
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_cv_from_parameter(const std::string& processor_name,
                                                 const std::string& parameter_name,
                                                 int cv_output_id) override;

    /**
     * @brief Connect a gate input to a processor. Gate changes will be sent as note
     *        on or note off messages to the processor on the selected channel and
     *        with the selected note number.
     * @param processor_name The unique name of the processor.
     * @param gate_input_id The gate input port to use.
     * @param note_no The note number to identify the gate id (0-127).
     * @param channel The channel to use (0-15).
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_gate_to_processor(const std::string& processor_name,
                                                 int gate_input_id,
                                                 int note_no,
                                                 int channel) override;

    /**
     * @brief Connect a processor to a gate output. Note on or note off messages sent
     *        from the processor on the selected channel and note number will be converted
     *        to gate events on the gate output.
     * @param processor_name The unique name of the processor.
     * @param gate_output_id The gate output port to use.
     * @param note_no The note number to identify the gate id (0-127)
     * @param channel The channel to use (0-15).
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_gate_from_processor(const std::string& processor_name,
                                                   int gate_output_id,
                                                   int note_no,
                                                   int channel) override;

    /**
     * @brief Use a selected gate input as sync input.
     * @param gate_input_id The gate input port to use as sync input.
     * @param ppq_ticks Number of ticks per quarternote.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_gate_to_sync(int gate_input_id,
                                            int ppq_ticks) override;

    /**
     * @brief Send sync pulses on a gate output.
     * @param gate_output_id The gate output to use.
     * @param ppq_ticks Number of pulses per quarternote.
     * @return EngineReturnStatus::OK if successful, error status otherwise
     */
    EngineReturnStatus connect_sync_to_gate(int gate_output_id,
                                            int ppq_ticks) override;

    /**
     * @brief Return the number of configured channels for a specific track
     *
     * @param track The index to the track
     * @return Number of channels the track is configured to use.
     */
    int n_channels_in_track(int track) override;

    /**
     * @brief Returns whether the engine is running in a realtime mode or not
     * @return true if the engine is currently processing in realtime mode, false otherwise
     */
    bool realtime() override;

    /**
     * @brief Set the engine to operate in realtime mode. In this mode process_chunk and
     *        send_rt_event is assumed to be called from a realtime thread.
     *        All other function calls are assumed to be made from non-realtime threads
     *
     * @param enabled true to enable realtime mode, false to disable
     */
    void enable_realtime(bool enabled) override;

    /**
     * @brief Process one chunk of audio from in_buffer and write the result to out_buffer
     * @param in_buffer input audio buffer
     * @param out_buffer output audio buffer
     * @param in_controls input control voltage and gate data
     * @param out_controls output control voltage and gate data
     */
    void process_chunk(SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer,
                       SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer,
                       ControlBuffer* in_controls,
                       ControlBuffer* out_controls) override;

    /**
     * @brief Set the current time for the start of the current audio chunk
     * @param timestamp Current time in microseconds
     * @param samples Current number of samples processed
     */
    void update_time(Time timestamp, int64_t samples) override
    {
        _transport.set_time(timestamp, samples);
    }

    /**
     * @brief Inform the engine of the current system latency
     * @param latency The output latency of the audio system
     */
    void set_output_latency(Time latency) override
    {
        _transport.set_latency(latency);
    }

    /**
     * @brief Set the tempo of the engine. Intended to be called from a non-thread.
     * @param tempo The new tempo in beats (quarter notes) per minute
     */
    void set_tempo(float tempo) override;

    /**
     * @brief Set the time signature of the engine. Intended to be called from a non-thread.
     * @param signature A TimeSignature object describing the new time signature to use
     */
    void set_time_signature(TimeSignature signature) override;

    /**
     * @brief Set the current transport mode, i.e stopped, playing, recording. This will be
     *        passed on to processors. Note stopped here means that audio is still running
     *        but sequencers and similiar should be in a stopped state.
     *        Currently only STOPPED and PLAYING are implemented and default is PLAYING
     * @param mode A TransportMode mode with the current state
     */
    void set_transport_mode(PlayingMode mode) override;

    /**
     * @brief Set the current mode of synchronising the engine tempo and beats. Default is
     *        INTERNAL.
     * @param mode A SyncMode with the current mode of syncronisation
     */
    void set_tempo_sync_mode(SyncMode mode) override;
    /**
     * @brief Process an event directly. In a realtime processing setup this must be
     *        called from the realtime thread before calling process_chunk()
     * @param event The event to process
     * @return EngineReturnStatus::OK if the event was properly processed, error code otherwise
     */
    EngineReturnStatus send_rt_event(RtEvent& event) override;

    /**
     * @brief Called from a non-realtime thread to process an event in the realtime
     * @param event The event to process
     * @return EngineReturnStatus::OK if the event was properly processed, error code otherwise
     */
    EngineReturnStatus send_async_event(RtEvent& event) override;
    /**
     * @brief Get the unique id of a processor given its name
     * @param unique_name The unique name of a processor
     * @return the unique id of the processor, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, ObjectId> processor_id_from_name(const std::string& name) override;

    /**
     * @brief Get the unique (per processor) id of a parameter.
     * @param processor_name The unique name of a processor
     * @param The unique name of a parameter of the above processor
     * @return the unique id of the parameter, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, ObjectId> parameter_id_from_name(const std::string& /*processor_name*/,
                                                                   const std::string& /*parameter_name*/);

    /**
     * @brief Get the unique name of a processor of a known unique id
     * @param uid The unique id of the processor
     * @return The name of the processor, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, const std::string> processor_name_from_id(const ObjectId uid) override;

    /**
     * @brief Get the unique name of a parameter with a known unique id
     * @param processor_name The unique name of the processor
     * @param id The unique id of the parameter to lookup.
     * @return The name of the processor, only valid if status is EngineReturnStatus::OK
     */
    std::pair<EngineReturnStatus, const std::string> parameter_name_from_id(const std::string& processor_name,
                                                                            const ObjectId id) override;
    /**
     * @brief Create an empty track
     * @param name The unique name of the track to be created.
     * @param channel_count The number of channels in the track.
     * @return EngineInitStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus create_track(const std::string& name, int channel_count) override;

    /**
     * @brief Create an empty track
     * @param name The unique name of the track to be created.
     * @param input_busses The number of input stereo pairs in the track.
     * @param output_busses The number of output stereo pairs in the track.
     * @return EngineInitStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus create_multibus_track(const std::string& name, int input_busses, int output_busses) override;
    /**
     * @brief Delete a track, currently assumes that the track is empty before calling
     * @param track_name The unique name of the track to delete
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus delete_track(const std::string &track_name) override;

    /**
     * @brief Creates and adds a plugin to a track.
     * @param track_id The unique id of the track to which the processor will be appended
     * @param plugin_uid The unique id of the plugin
     * @param plugin_name The name to give the plugin after loading
     * @param plugin_path The file to load the plugin from, only valid for external plugins
     * @param plugin_type The type of plugin, i.e. internal or external
     * @return EngineReturnStatus::OK in case of success, different error code otherwise.
     */
    EngineReturnStatus add_plugin_to_track(const std::string &track_name,
                                           const std::string &plugin_uid,
                                           const std::string &plugin_name,
                                           const std::string &plugin_path,
                                           PluginType plugin_type) override;

    /**
     * @brief Remove a given plugin from a track and delete it
     * @param track_name The unique name of the track that contains the plugin
     * @param plugin_name The unique name of the plugin
     * @return EngineReturnStatus::OK in case of success, different error code otherwise
     */
    EngineReturnStatus remove_plugin_from_track(const std::string &track_name,
                                                const std::string &plugin_name) override;

    /**
     * @brief Access a particular processor by its unique id for querying
     * @param processor_id The id of the processor
     * @return A const pointer to the processor instance if found, nullptr otherwise
     */
    const Processor* processor(ObjectId processor_id) const override;

    /**
     * @brief Access a particular processor by its unique id for editing,
     *        use with care and not from several threads at once
     * @param processor_id The id of the processor
     * @return A mutable pointer to the processor instance if found, nullptr otherwise
     */
    Processor* mutable_processor(ObjectId processor_id) override;

    /**
     * @brief Return all processors. Potentially dangerous so use with care and eventually
     *        there should be better and safer ways of accessing processors.
     * @return An std::map containing all registered processors.
     */
    const std::map<std::string, std::unique_ptr<Processor>>& all_processors() override
    {
        return _processors;
    };

    /**
     * @brief Return all tracks. Potentially unsafe so use with care. Should
     *        eventually be replaces with a better way of accessing tracks/processors
     *        from outside the engine.
     * @return An std::vector of containing all Tracks
     */
    const std::vector<Track*>& all_tracks() override
    {
        return _audio_graph;
    }

    /**
     * @brief Enable audio clip detection on engine inputs
     * @param enabled Enable if true, disable if false
     */
    void enable_input_clip_detection(bool enabled) override
    {
        _input_clip_detection_enabled = enabled;
    }

    /**
     * @brief Enable audio clip detection on engine outputs
     * @param enabled Enable if true, disable if false
     */
    void enable_output_clip_detection(bool enabled) override
    {
        _output_clip_detection_enabled = enabled;
    }

    sushi::dispatcher::BaseEventDispatcher* event_dispatcher() override
    {
        return &_event_dispatcher;
    }

    sushi::ext::SushiControl* controller() override
    {
        return &_controller;
    }

    sushi::engine::Transport* transport() override
    {
        return &_transport;
    }

    performance::BasePerformanceTimer* performance_timer() override
    {
        return &_process_timer;
    }

    /**
     * @brief Print the current processor timings (in enabled) in the log
     */
    void print_timings_to_log() override;

private:
    /**
     * @brief Instantiate a plugin instance of a given type
     * @param uid String unique id
     * @return Pointer to plugin instance if uid is valid, nullptr otherwise
     */
    Processor* _make_internal_plugin(const std::string& uid);

    /**
     * @brief Register a newly created processor in all lookup containers
     *        and take ownership of it.
     * @param processor Processor to register
     */
    EngineReturnStatus _register_processor(Processor* processor, const std::string& name);

    /**
     * @breif Remove a processor from the engine and delete it.
     * @param name The unique name of the processor to delete
     * @return True if the processor existed and it was correctly deleted
     */
    EngineReturnStatus _deregister_processor(const std::string& name);

    /**
     * @brief Add a registered processor to the realtime processing part.
     *        Must be called before a processor can be used to process audio.
     * @param processor Processor to insert
     */
    bool _insert_processor_in_realtime_part(Processor* processor);

    /**
     * @brief Remove a processor from the realtime processing part
     *        Must be called before de-registering a processor.
     * @param name The unique name of the processor to delete
     * @return True if the processor existed and it was correctly deleted
     */
    bool _remove_processor_from_realtime_part(ObjectId processor);

    /**
     * @brief Register a newly created track
     * @param track Pointer to the track
     * @return OK if succesfull, error code otherwise
     */
    EngineReturnStatus _register_new_track(const std::string& name, Track* track);

    /**
     * @brief Checks whether a processor exists in the engine.
     * @param processor_name The unique name of the processor.
     * @return Returns true if exists, false if it does not.
     */
    bool _processor_exists(const std::string& processor_name);

    /**
     * @brief Checks whether a processor exists in the engine.
     * @param uid The unique id of the processor.
     * @return Returns true if exists, false if it does not.
     */
    bool _processor_exists(ObjectId uid);

    /**
     * @brief Process events that are to be handles by the engine directly and
     *        not by a particular processor.
     * @param event The event to handle
     * @return true if handled, false if not an engine event
     */
    bool _handle_internal_events(RtEvent &event);

    inline void _retrieve_events_from_tracks(ControlBuffer& buffer);

    inline void _copy_audio_to_tracks(ChunkSampleBuffer* input);

    inline void _copy_audio_from_tracks(ChunkSampleBuffer* output);

    void print_timings_to_file(const std::string& filename);

    void _route_cv_gate_ins(ControlBuffer& buffer);

    void _process_outgoing_events(ControlBuffer& buffer, RtSafeRtEventFifo& source_queue);

    const bool _multicore_processing;
    const int  _rt_cores;

    std::unique_ptr<twine::WorkerPool> _worker_pool;

    std::vector<Track*> _audio_graph;

    // All registered processors indexed by their unique name
    std::map<std::string, std::unique_ptr<Processor>> _processors;

    // Processors in the realtime part indexed by their unique 32 bit id
    // Only to be accessed from the process callback in rt mode.
    std::vector<Processor*> _realtime_processors{MAX_RT_PROCESSOR_ID, nullptr};

    struct AudioConnection
    {
        int engine_channel;
        int track_channel;
        ObjectId track;
    };
    std::vector<AudioConnection> _in_audio_connections;
    std::vector<AudioConnection> _out_audio_connections;

    struct CvConnection
    {
        ObjectId processor_id;
        ObjectId parameter_id;
        int cv_id;
    };

    struct GateConnection
    {
        ObjectId processor_id;
        int gate_id;
        int note_no;
        int channel;
    };

    std::vector<CvConnection> _cv_in_routes;
    std::vector<GateConnection> _gate_in_routes;
    BitSet32 _prev_gate_values{0};
    BitSet32 _outgoing_gate_values{0};

    std::atomic<RealtimeState> _state{RealtimeState::STOPPED};

    RtSafeRtEventFifo _internal_control_queue;
    RtSafeRtEventFifo _main_in_queue;
    RtSafeRtEventFifo _processor_out_queue;
    RtSafeRtEventFifo _main_out_queue;
    RtSafeRtEventFifo _control_queue_out;
    std::mutex _in_queue_lock;
    receiver::AsynchronousEventReceiver _event_receiver{&_control_queue_out};
    Transport _transport;

    dispatcher::EventDispatcher _event_dispatcher{this, &_main_out_queue, &_main_in_queue};
    Controller _controller{this};

    HostControl _host_control{&_event_dispatcher, &_transport};
    performance::PerformanceTimer _process_timer;
    bool _timings_enabled{false};

    bool _input_clip_detection_enabled{false};
    bool _output_clip_detection_enabled{false};
    ClipDetector _clip_detector;
};

/**
 * @brief Helper function to encapsulate state changes from transient states
 * @param current_state The current state of the engine
 * @return A new, non-transient state
 */
RealtimeState update_state(RealtimeState current_state);

} // namespace engine
} // namespace sushi
#endif //SUSHI_ENGINE_H
