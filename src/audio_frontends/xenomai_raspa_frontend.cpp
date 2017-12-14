/**
 * @brief Frontend using Xenomai with RASPA library for XMOS board.
 *
 */
#ifdef SUSHI_BUILD_WITH_XENOMAI

#include <unistd.h>
#include <errno.h>

#include "raspa.h"

#include "xenomai_raspa_frontend.h"
#include "logging.h"

namespace sushi {
namespace audio_frontend {

MIND_GET_LOGGER;

AudioFrontendStatus XenomaiRaspaFrontend::init(BaseAudioFrontendConfiguration* config)
{
    auto ret_code = BaseAudioFrontend::init(config);
    if (ret_code != AudioFrontendStatus::OK)
    {
        return ret_code;
    }

    // RASPA
    if (RASPA_N_FRAMES_PER_BUFFER != AUDIO_CHUNK_SIZE)
    {
        MIND_LOG_ERROR("Chunk size mismatch, check driver configuration.");
        return AudioFrontendStatus::INVALID_CHUNK_SIZE;
    }
    if (RASPA_N_CHANNELS != MAX_FRONTEND_CHANNELS)
    {
        MIND_LOG_ERROR("Number of channels mismatch, check driver configuration.");
        return AudioFrontendStatus::INVALID_N_CHANNELS;
    }
    _engine->set_audio_input_channels(RASPA_N_CHANNELS);
    _engine->set_audio_output_channels(RASPA_N_CHANNELS);

    auto init_ret = raspa_init();
    if (init_ret < 0)
    {
        MIND_LOG_ERROR("Error initializing RASPA: {}", strerror(-init_ret));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    auto raspa_ret = raspa_open(RASPA_N_CHANNELS, RASPA_N_FRAMES_PER_BUFFER, rt_process_callback, this);
    if (raspa_ret < 0)
    {
        MIND_LOG_ERROR("Error opening RASPA: {}", strerror(-raspa_ret));
        return AudioFrontendStatus::AUDIO_HW_ERROR;
    }

    // Control
    _osc_control = std::make_unique<control_frontend::OSCFrontend>(_engine);
    _midi_frontend = std::make_unique<midi_frontend::AlsaMidiFrontend>(_midi_dispatcher);
    auto midi_ok = _midi_frontend->init();
    if (!midi_ok)
    {
        return AudioFrontendStatus::MIDI_PORT_ERROR;
    }
    return AudioFrontendStatus::OK;
}


void XenomaiRaspaFrontend::cleanup()
{
    _osc_control->stop();
    _midi_frontend->stop();
    MIND_LOG_INFO("Closing Raspa driver.");
    raspa_close();
}


void XenomaiRaspaFrontend::run()
{
    _osc_control->run();
    _osc_control->connect_all();
    _midi_frontend->run();
}


void XenomaiRaspaFrontend::_internal_process_callback(float* input, float* output)
{
    while (!_event_queue.empty())
    {
        RtEvent event;
        if (_event_queue.pop(event))
        {
            _engine->send_rt_event(event);
        }
    }

    ChunkSampleBuffer in_buffer = ChunkSampleBuffer::create_from_raw_pointer(input, 0, RASPA_N_CHANNELS);
    ChunkSampleBuffer out_buffer = ChunkSampleBuffer::create_from_raw_pointer(output, 0, RASPA_N_CHANNELS);
    out_buffer.clear();
    _engine->process_chunk(&in_buffer, &out_buffer);
}


}; // end namespace audio_frontend
}; // end namespace sushi

#else // SUSHI_BUILD_WITH_XENOMAI

#include <cassert>
#include "audio_frontends/xenomai_raspa_frontend.h"
#include "logging.h"
namespace sushi {
namespace audio_frontend {
MIND_GET_LOGGER;
XenomaiRaspaFrontend::XenomaiRaspaFrontend(engine::BaseEngine* engine,
                                midi_dispatcher::MidiDispatcher* midi_dispatcher) : BaseAudioFrontend(engine, midi_dispatcher)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    MIND_LOG_ERROR("Sushi was not built with Xenomai Cobalt support!");
    assert(false);
}}}

#endif // SUSHI_BUILD_WITH_XENOMAI