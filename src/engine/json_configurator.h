/**
 * @brief Class to configure the audio engine using Json config files.
 * @copyright MIND Music Labs AB, Stockholm
 *
 * @details This file contains class JsonConfigurator which provides public methods
 * to read configuration data from Json config files, validate its schema and configure
 * the engine with it. It can load tracks and midi connections from the config file.
 */

#ifndef SUSHI_CONFIG_FROM_JSON_H
#define SUSHI_CONFIG_FROM_JSON_H

#include <optional>

#include "rapidjson/document.h"

#include "base_engine.h"
#include "engine/midi_dispatcher.h"

namespace sushi {
namespace jsonconfig {

enum class JsonConfigReturnStatus
{
    OK,
    INVALID_CONFIGURATION,
    INVALID_TRACK_NAME,
    INVALID_PLUGIN_PATH,
    INVALID_PARAMETER,
    INVALID_PLUGIN_NAME,
    INVALID_MIDI_PORT,
    INVALID_FILE,
    NO_MIDI_DEFINITIONS,
    NO_CV_GATE_DEFINITIONS,
    NO_EVENTS_DEFINITIONS
};

enum class JsonSection
{
    HOST_CONFIG,
    TRACKS,
    MIDI,
    CV_GATE,
    EVENTS
};

struct AudioConfig
{
    std::optional<int> cv_inputs;
    std::optional<int> cv_outputs;
};

class JsonConfigurator
{
public:
    JsonConfigurator(engine::BaseEngine* engine,
                     midi_dispatcher::MidiDispatcher* midi_dispatcher,
                     const std::string& path) : _engine(engine),
                                                _midi_dispatcher(midi_dispatcher),
                                                _document_path(path){}

    ~JsonConfigurator() {}

    /**
     * @brief Reads the json config  and returns all audio frontend configuration options
     *        that are not set on the audio engine directly
     * @return A tuple of status and AudioConfig struct, AudioConfig is only valid if status is
     *         JsonConfigReturnStatus::OK
     */
    std::pair<JsonConfigReturnStatus, AudioConfig> load_audio_config();

    /**
     * @brief Reads the json config  and set the given host configuration options
     * @param path_to_file String which denotes the path of the file.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_host_config();

    /**
     * @brief Reads the json config , searches for valid tracks
     *        definitions and configures the engine with the specified tracks.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_tracks();

    /**
     * @brief Reads the json config , searches for valid MIDI connections and
     *        MIDI CC Mapping definitions and configures the engine with the specified MIDI information.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_midi();

    /**
     * @brief Reads the json config , searches for valid control voltage and gate
     *        connection definitions and configures the engine with the specified routing.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_cv_gate();

    /**
     * @brief Reads the json config , searches for a valid "events" definition and
     *        queues them to the engines internal queue.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus load_events();

    /**
     * @brief Reads the json config , searches for a valid "events" definition and
     *        returns all parsed events as a list
     * @return An std::vector with the parsed events which is only valid if the status
     *         returned is JsonConfigReturnStatus::OK
     */
    std::pair<JsonConfigReturnStatus, std::vector<Event*>> load_event_list();

private:
    /**
     * @brief Helper function to retrieve a particular section of the json configuration
     * @param section Jsonsection to denote which section is to be validated.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    std::pair<JsonConfigReturnStatus, const rapidjson::Value&> _parse_section(JsonSection section);

    /**
     * @brief Uses Engine's API to create a single track with the specified number of channels and adds
     *        the respective plugins to the track if they are defined in the file. Used by load_tracks.
     * @param track_def rapidjson document object representing a single track and its details.
     * @return JsonConfigReturnStatus::OK if success, different error code otherwise.
     */
    JsonConfigReturnStatus _make_track(const rapidjson::Value &track_def);

    /**
     * @brief Helper function to extract the number of midi channels in the midi definition.
     * @param channels rapidjson document object containing the channel information parsed from the file.
     * @return The number of MIDI channels.
     */
    int _get_midi_channel(const rapidjson::Value& channels);

    /* Helper enum for more expressive code */
    enum EventParseMode : bool
    {
        IGNORE_TIMESTAMP = false,
        USE_TIMESTAMP = true,
    };
    /**
     * @brief Helper function to parse a single event
     * @param json_event A json value representing an event
     * @param with_timestamp If set to true, the timestamp from the json definition will be used
     *        if set to false, the event timestamp will be set for immediate processing
     * @return A pointer to an Event if successful, nullptr otherwise
     */
    Event* _parse_event(const rapidjson::Value& json_event, bool with_timestamp);

    /**
     * @brief function which validates the json data against the respective schema.
     * @param config rapidjson document object containing the json data parsed from the file
     * @param section JsonSection to denote which json section is to be validated.
     * @return true if json follows schema, false otherwise
     */
    bool _validate_against_schema(rapidjson::Value& config, JsonSection section);

    JsonConfigReturnStatus _load_data();

    engine::BaseEngine* _engine;
    midi_dispatcher::MidiDispatcher* _midi_dispatcher;

    std::string _document_path;
    rapidjson::Document _json_data;
};

}/* namespace JSONCONFIG */
}/* namespace SUSHI */

#endif //SUSHI_CONFIG_FROM_JSON_H