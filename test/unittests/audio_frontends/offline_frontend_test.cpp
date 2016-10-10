#include "gtest/gtest.h"
#include "audio_frontends/offline_frontend.cpp"
#include "engine_mockup.h"

using ::testing::internal::posix::GetEnv;

using namespace sushi;
using namespace sushi::audio_frontend;

static constexpr unsigned int SAMPLE_RATE = 44000;

class TestOfflineFrontend : public ::testing::Test
{
protected:
    TestOfflineFrontend()
    {
    }

    void SetUp()
    {
        _module_under_test = new OfflineFrontend(&_engine);
    }

    void TearDown()
    {
        delete _module_under_test;
    }

    EngineMockup _engine{SAMPLE_RATE};
    OfflineFrontend* _module_under_test;
};

TEST_F(TestOfflineFrontend, test_wav_processing)
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable";
    }

    // Initialize with a file cointaining 0.5 on both channels
    std::string test_data_file(test_data_dir);
    test_data_file.append("/test_sndfile_05.wav");
    std::string output_file_name("./test_out.wav");
    OfflineFrontendConfiguration config(test_data_file, "./test_out.wav");
    auto ret_code = _module_under_test->init(&config);
    if (ret_code != AudioFrontendInitStatus::OK)
    {
        EXPECT_TRUE(false) << "Error initializing Frontend";
    }
    // Process with the dummy bypass engine
    _module_under_test->run();

    // Read the generated file and verify the result
    SNDFILE*    output_file;
    SF_INFO     soundfile_info;

    if (! (output_file = sf_open(output_file_name.c_str(), SFM_READ, &soundfile_info)) )
    {
        EXPECT_TRUE(false) << "Error opening output file: " << output_file_name;
    }

    float file_buffer[_engine.n_channels() * AUDIO_CHUNK_SIZE];
    unsigned int readcount;
    while ( (readcount = static_cast<unsigned int>(sf_readf_float(output_file,
                                                         &file_buffer[0],
                                                         static_cast<sf_count_t>(AUDIO_CHUNK_SIZE)))) )
    {
        for (unsigned int n=0; n<(readcount * _engine.n_channels()); n++)
        {
            ASSERT_FLOAT_EQ(0.5f, file_buffer[n]);
        }
    }

    sf_close(output_file);
}

TEST_F(TestOfflineFrontend, test_invalid_input_file)
{
    OfflineFrontendConfiguration config("this_is_not_a_valid_file.extension", "./test_out.wav");
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendInitStatus::INVALID_INPUT_FILE, ret_code);
}

TEST_F(TestOfflineFrontend, test_channel_match)
{
    char const* test_data_dir = GetEnv("SUSHI_TEST_DATA_DIR");
    if (test_data_dir == nullptr)
    {
        EXPECT_TRUE(false) << "Can't access Test Data environment variable";
    }

    // Initialize with a file cointaining 0.5 on both channels
    std::string test_data_file(test_data_dir);
    test_data_file.append("/mono.wav");
    OfflineFrontendConfiguration config(test_data_file, "./test_out.wav");
    auto ret_code = _module_under_test->init(&config);
    ASSERT_EQ(AudioFrontendInitStatus::INVALID_N_CHANNELS, ret_code);
}