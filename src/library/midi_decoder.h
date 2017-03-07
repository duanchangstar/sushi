/**
 * @brief Utility functions for decoding raw midi data.
 *        To decode raw midi messages, first call decode_message_type
 *        which returns the type and checks that the length is correct
 *        for the type. Then you can safely call the corresponding
 *        decode function for that type of message.
 */

#ifndef SUSHI_MIDI_DECODER_H
#define SUSHI_MIDI_DECODER_H

namespace sushi {
namespace midi {

/* Max value for midi velocity, pressure, controller value, etc. */
constexpr int MAX_VALUE = 127;
/* Max value for midi pitch bend (14 bit value). */
constexpr int MAX_PITCH_BEND = 16384;
/* Middle value for pitch bend */
constexpr int PITCH_BEND_MIDDLE = 8192;


/**
 * @brief Enum to represent a midi message type.
 */
enum class MessageType
{
    /* Channel voice messages */
    NOTE_OFF,
    NOTE_ON,
    POLY_KEY_PRESSURE,
    CONTROL_CHANGE,
    PROGRAM_CHANGE,
    CHANNEL_PRESSURE,
    PITCH_BEND,
    /* Channel mode messages */
    ALL_SOUND_OFF,
    RESET_ALL_CONTROLLERS,
    LOCAL_CONTROL_ON,
    LOCAL_CONTROL_OFF,
    ALL_NOTES_OFF,
    OMNI_MODE_OFF,
    OMNI_MODE_ON,
    MONO_MODE_ON,
    POLY_MODE_ON,
    /* System common messages */
    SYSTEM_EXCLUSIVE,
    TIME_CODE,
    SONG_POSITION,
    SONG_SELECT,
    TUNE_REQUEST,
    END_OF_EXCLUSIVE,
    /* System real time messages */
    TIMING_CLOCK,
    START,
    CONTINUE,
    STOP,
    ACTIVE_SENSING,
    RESET,
    /* Unhandled or corrupt messages */
    UNKNOWN
};

struct NoteOffMessage
{
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
};

struct NoteOnMessage
{
    uint8_t channel;
    uint8_t note;
    uint8_t velocity;
};

struct PolyKeyPressureMessage
{
    uint8_t channel;
    uint8_t note;
    uint8_t pressure;
};

struct ControlChangeMessage
{
    uint8_t channel;
    uint8_t controller;
    uint8_t value;
};

struct ProgramChangeMessage
{
    uint8_t channel;
    uint8_t program;
};

struct ChannelPressureMessage
{
    uint8_t channel;
    uint8_t pressure;
};

struct PitchBendMessage
{
    uint8_t channel;
    uint16_t value;
};

struct TimeCodeMessage
{
    uint8_t message_type;
    uint8_t value;
};

struct SongPositionMessage
{
    uint16_t beats; /* No of beats since start of song, 1 beat = 6 midi clock ticks.*/
};

struct SongSelectMessage
{
    uint8_t index;
};

/**
 * @brief Decode the type of midi message from a byte string
 * @param data Midi data.
 * @param size Length of midi data.
 * @return The decoded type of data.
 */
MessageType decode_message_type(const uint8_t* data, size_t size);

/**
 * @brief Decode the channel number of a channel mode message.
 *        I.e. when the decoded message type is between
 *        ALL_SOUND_OFF and POLY_MODE_ON.
 * @param data First byte of the midi message.
 * @return The decoded channel number (0-15) from the given message
 */
uint8_t decode_channel(const uint8_t data);

/**
 * @brief Decode a midi note off message.
 * @param data Message data.
 * @return Decoded message.
 */
NoteOffMessage decode_note_off(const uint8_t* data);

/**
 * @brief Decode a midi note on message.
 * @param data Message data.
 * @return Decoded message.
 */
NoteOnMessage decode_note_on(const uint8_t* data);

/**
 * @brief Decode a midi control change message.
 * @param data Message data.
 * @return Decoded message.
 */
ControlChangeMessage decode_control_change(const uint8_t* data);

/**
 * @brief Decode a midi polyphonic key pressure (aftertouch) message.
 * @param data Message data.
 * @return Decoded message.
 */
PolyKeyPressureMessage decode_poly_key_pressure(const uint8_t* data);

/**
 * @brief Decode a midi program change message.
 * @param data Message data.
 * @return Decoded message.
 */
ProgramChangeMessage decode_program_change(const uint8_t* data);

/**
 * @brief Decode a midi channel pressure (non-polyphonic aftertouch) message.
 * @param data Message data.
 * @return Decoded message.
 */
ChannelPressureMessage decode_channel_pressure(const uint8_t* data);

/**
 * @brief Decode a midi pitch bend message.
 * @param data Message data.
 * @return Decoded message.
 */
PitchBendMessage decode_pitch_bend(const uint8_t* data);

/**
 * @brief Decode a Midi time code quarter frame message.
 * @param data  Message data.
 * @return Decoded message.
 */
TimeCodeMessage decode_time_code(const uint8_t* data);

/**
 * @brief Decode a Midi song position message.
 * @param data  Message data.
 * @return Decoded message.
 */
SongPositionMessage decode_song_position(const uint8_t* data);

/**
 * @brief Decode a Midi song select message.
 * @param data  Message data.
 * @return Decoded message.
 */
SongSelectMessage decode_song_select(const uint8_t* data);


} // end namespace midi
} // end namespace sushi


#endif //SUSHI_MIDI_DECODER_H