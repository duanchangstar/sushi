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
 * @brief Handles translation of midi to internal events and midi routing
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_MIDI_DISPATCHER_H
#define SUSHI_MIDI_DISPATCHER_H

#include <string>
#include <map>
#include <array>
#include <vector>

#include "library/constants.h"
#include "library/types.h"
#include "library/midi_decoder.h"
#include "library/event.h"
#include "library/processor.h"
#include "control_frontends/base_midi_frontend.h"
#include "base_engine.h"
#include "base_event_dispatcher.h"
#include "midi_receiver.h"
#include "library/event_interface.h"

namespace sushi {
namespace midi_dispatcher {

struct InputConnection
{
    ObjectId target;
    ObjectId parameter;
    float min_range;
    float max_range;
    bool relative;
    uint8_t virtual_abs_value;
};

struct OutputConnection
{
    int channel;
    int output;
    int cc_number;
    float min_range;
    float max_range;
};

enum class MidiDispatcherStatus
{
    OK,
    INVALID_MIDI_INPUT,
    INVALID_MIDI_OUTPUT,
    INVALID_CHAIN_NAME,
    INVALID_PROCESSOR,
    INVALID_PARAMETER,
    INVAlID_CHANNEL
};

class MidiDispatcher : public EventPoster, public midi_receiver::MidiReceiver
{
    SUSHI_DECLARE_NON_COPYABLE(MidiDispatcher);

public:
    MidiDispatcher(engine::BaseEngine* engine);

    virtual ~MidiDispatcher();

    // TODO - Eventually have the frontend as a constructor argument
    // Doesn't work now since the dispatcher is created in main
    void set_frontend(midi_frontend::BaseMidiFrontend* frontend)
    {
        _frontend = frontend;
    }
/**
 * @brief Sets the number of midi input ports.
 * @param ports number of input ports.
 */
    void set_midi_inputs(int no_inputs)
    {
        _midi_inputs = no_inputs;
    }

    /**
     * @brief Sets the number of midi output ports.
     * @param ports number of output ports.
     */
    void set_midi_outputs(int no_outputs)
    {
        _midi_outputs = no_outputs;
    }

    /**
     * @brief Connects a midi control change message to a given parameter.
     *        Eventually you should be able to set range, curve etc here.
     * @brief midi_input Index to the registered midi output.
     * @param processor The processor target
     * @param parameter The parameter to map to
     * @param cc_no The cc id to use
     * @param min_range Minimum range for this controller
     * @param max_range Maximum range for this controller
     * @param channel If not OMNI, only the given channel will be connected.
     * @return true if successfully forwarded midi message
     */
    MidiDispatcherStatus connect_cc_to_parameter(int midi_input,
                                                 const std::string &processor_name,
                                                 const std::string &parameter_name,
                                                 int cc_no,
                                                 float min_range,
                                                 float max_range,
                                                 bool use_relative_mode,
                                                 int channel = midi::MidiChannel::OMNI);

    /**
    * @brief Connects midi program change messages to a processor.
    * @brief midi_input Index to the registered midi output.
    * @param processor The processor target
    * @param channel If not OMNI, only the given channel will be connected.
    * @return true if successfully forwarded midi message
    */
    MidiDispatcherStatus connect_pc_to_processor(int midi_input,
                                                 const std::string &processor_name,
                                                 int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Connect a midi input to a track
     *        Possibly filtering on midi channel.
     * @param midi_input Index of the midi input
     * @param track_name The track/processor track to send to
     * @param channel If not OMNI, only the given channel will be connected.
     * @return OK if successfully connected the track, error status otherwise
     */
    MidiDispatcherStatus connect_kb_to_track(int midi_input,
                                             const std::string &track_name,
                                             int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Connect a midi input to a track and send unprocessed
     *        Midi data to it. Possibly filtering on midi channel.
     * @param midi_input Index of the midi input
     * @param track_name The track/processor track to send to
     * @param channel If not OMNI, only the given channel will be connected.
     * @return OK if successfully connected the track, error status otherwise
     */
    MidiDispatcherStatus connect_raw_midi_to_track(int midi_input,
                                                   const std::string &track_name,
                                                   int channel = midi::MidiChannel::OMNI);

    /**
     * @brief Connect midi kb data from a track to a given midi output
     * @param midi_output Index of the midi out
     * @param track_name The track/processor track from where the data originates
     * @param channel Which channel nr to output the data on
     * @return OK if successfully connected the track, error status otherwise
     */
    MidiDispatcherStatus connect_track_to_output(int midi_output,
                                                 const std::string &track_name,
                                                 int channel);
    /**
     * @brief Clears all connections made with connect_kb_to_track
     *        and connect_cc_to_parameter.
     */
    void clear_connections();

    /**
     * @brief Process a raw midi message and send it of according to the
     *        configured connections.
     * @param port Index of the originating midi port.
     * @param data Pointer to the raw midi message.
     * @param size Length of data in bytes.
     * @param timestamp timestamp of the midi event
     */
    void send_midi(int port, MidiDataByte data, Time timestamp) override;

    /* Inherited from EventPoster */
    int process(Event* /*event*/) override;

    /**
     * @brief The unique id of this poster.
     * @return
     */
    int poster_id() override {return EventPosterId::MIDI_DISPATCHER;}

private:

    std::map<int, std::array<std::vector<InputConnection>, midi::MidiChannel::OMNI + 1>> _kb_routes_in;
    std::map<ObjectId, std::vector<OutputConnection>>  _kb_routes_out;
    std::map<int, std::array<std::array<std::vector<InputConnection>, midi::MidiChannel::OMNI + 1>, midi::MAX_CONTROLLER_NO + 1>> _cc_routes;
    std::map<int, std::array<std::vector<InputConnection>, midi::MidiChannel::OMNI + 1>> _pc_routes;
    std::map<int, std::array<std::vector<InputConnection>, midi::MidiChannel::OMNI + 1>> _raw_routes_in;
    int _midi_inputs{0};
    int _midi_outputs{0};

    engine::BaseEngine* _engine;
    midi_frontend::BaseMidiFrontend* _frontend;
    dispatcher::BaseEventDispatcher* _event_dispatcher;
};

} // end namespace midi_dispatcher
} // end namespace sushi

#endif //SUSHI_MIDI_DISPATCHER_H
