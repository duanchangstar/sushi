/**
 * @brief Alsa midi frontend
 * @copyright MIND Music Labs AB, Stockholm
 *
 * This module provides a frontend for getting midi messages into the engine
 */
#ifndef SUSHI_ALSA_MIDI_FRONTEND_H
#define SUSHI_ALSA_MIDI_FRONTEND_H

#include <thread>
#include <atomic>

#include <alsa/asoundlib.h>

#include "base_midi_frontend.h"
#include "library/time.h"

namespace sushi {
namespace midi_frontend {

constexpr int ALSA_EVENT_MAX_SIZE = 12;

class AlsaMidiFrontend : public BaseMidiFrontend
{
public:
    AlsaMidiFrontend(midi_receiver::MidiReceiver* dispatcher);

    ~AlsaMidiFrontend();

    bool init() override;

    void run() override;

    void stop() override;

    void send_midi(int input, MidiDataByte data, Time timestamp) override;

private:

    bool _init_ports();
    bool _init_time();
    Time _to_sushi_time(const snd_seq_real_time_t* alsa_time);
    snd_seq_real_time_t _to_alsa_time(Time timestamp);

    void                        _poll_function();
    std::thread                 _worker;
    std::atomic<bool>           _running{false};
    snd_seq_t*                  _seq_handle{nullptr};
    int                         _input_midi_port;
    int                         _output_midi_port;
    int                         _queue;

    snd_midi_event_t*           _input_parser{nullptr};
    snd_midi_event_t*           _output_parser{nullptr};
    Time                        _time_offset;
};

} // end namespace midi_frontend
} // end namespace sushi

#endif //SUSHI_ALSA_MIDI_FRONTEND_H_H
