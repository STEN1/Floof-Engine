#include "SoundManager.h"
//#include <AL/al.h>
//#include "alc.h"



#define alCall(function, ...) alCallImpl(__FILE__, __LINE__, function, __VA_ARGS__)
#define alcCall(function, device, ...) alcCallImpl(__FILE__, __LINE__, function, device, __VA_ARGS__)

// Check for errors
#define OpenAL_ErrorCheck(message) {\
    ALenum error = alGetError();\
    if (error != AL_NO_ERROR) std::cerr << "OpenAL Error: " << error << " with call for " << #message << std::endl;\
}

// Macro to route function calls into to check for errors
#define alec(FUNCTION_CALL)\
    FUNCTION_CALL;\
    OpenAL_ErrorCheck(FUNCTION_CALL)

#define DR_WAV_IMPLEMENTATION
#include <dr_libs/dr_wav.h>

namespace FLOOF {
	SoundManager::SoundManager()
	{
        ALCdevice* openALDevice = alcOpenDevice(nullptr);
        if (!openALDevice)
            return;

        ALCcontext* openALContext;
        if (!alcCall(alcCreateContext, openALContext, openALDevice, openALDevice, nullptr) || !openALContext)
        {
            std::cerr << "ERROR: Could not create audio context" << std::endl;
            return;
        }
        ALCboolean contextMadeCurrent = false;
        if (!alcCall(alcMakeContextCurrent, contextMadeCurrent, openALDevice, openALContext)
            || contextMadeCurrent != ALC_TRUE)
        {
            std::cerr << "ERROR: Could not make audio context current" << std::endl;
            return;
        }

        std::uint8_t channels;
        std::int32_t sampleRate;
        std::uint8_t bitsPerSample;
        ALsizei dataSize;
        char* rawSoundData = load_wav("Assets/test.wav", channels, sampleRate, bitsPerSample, dataSize);
        if (rawSoundData == nullptr || dataSize == 0)
        {
            std::cerr << "ERROR: Could not load wav" << std::endl;
            return;
        }

        std::vector<char> soundData(rawSoundData, rawSoundData + dataSize);

        ALuint buffer;
        alCall(alGenBuffers, 1, &buffer);

        ALenum format;
        if (channels == 1 && bitsPerSample == 8)
            format = AL_FORMAT_MONO8;
        else if (channels == 1 && bitsPerSample == 16)
            format = AL_FORMAT_MONO16;
        else if (channels == 2 && bitsPerSample == 8)
            format = AL_FORMAT_STEREO8;
        else if (channels == 2 && bitsPerSample == 16)
            format = AL_FORMAT_STEREO16;
        else
        {
            std::cerr
                << "ERROR: unrecognised wave format: "
                << channels << " channels, "
                << bitsPerSample << " bps" << std::endl;
            return;
        }

        alCall(alBufferData, buffer, format, soundData.data(), soundData.size(), sampleRate);
        soundData.clear(); // erase the sound in RAM

        ALuint source;
        alCall(alGenSources, 1, &source);
        alCall(alSourcef, source, AL_PITCH, 1);
        alCall(alSourcef, source, AL_GAIN, 1.0f);
        alCall(alSource3f, source, AL_POSITION, 0, 0, 0);
        alCall(alSource3f, source, AL_VELOCITY, 0, 0, 0);
        alCall(alSourcei, source, AL_LOOPING, AL_FALSE);
        alCall(alSourcei, source, AL_BUFFER, buffer);

        alCall(alSourcePlay, source);

        ALint state = AL_PLAYING;

        while (state == AL_PLAYING)
        {
            alCall(alGetSourcei, source, AL_SOURCE_STATE, &state);
        }

        alCall(alDeleteSources, 1, &source);
        alCall(alDeleteBuffers, 1, &buffer);

        alcCall(alcMakeContextCurrent, contextMadeCurrent, openALDevice, nullptr);
        alcCall(alcDestroyContext, openALDevice, openALContext);

        ALCboolean closed;
        alcCall(alcCloseDevice, closed, openALDevice, openALDevice);

        return;
	}

	bool SoundManager::check_al_errors(const std::string& filename, const std::uint_fast32_t line)
	{
        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
        {
            std::cerr << "***ERROR*** (" << filename << ": " << line << ")\n";
            switch (error)
            {
            case AL_INVALID_NAME:
                std::cerr << "AL_INVALID_NAME: a bad name (ID) was passed to an OpenAL function";
                break;
            case AL_INVALID_ENUM:
                std::cerr << "AL_INVALID_ENUM: an invalid enum value was passed to an OpenAL function";
                break;
            case AL_INVALID_VALUE:
                std::cerr << "AL_INVALID_VALUE: an invalid value was passed to an OpenAL function";
                break;
            case AL_INVALID_OPERATION:
                std::cerr << "AL_INVALID_OPERATION: the requested operation is not valid";
                break;
            case AL_OUT_OF_MEMORY:
                std::cerr << "AL_OUT_OF_MEMORY: the requested operation resulted in OpenAL running out of memory";
                break;
            default:
                std::cerr << "UNKNOWN AL ERROR: " << error;
            }
            std::cerr << std::endl;
            return false;
        }
        return true;
	}

	bool SoundManager::check_alc_errors(const std::string& filename, const std::uint_fast32_t line, ALCdevice* device) {

        ALCenum error = alcGetError(device);
        if (error != ALC_NO_ERROR)
        {
            std::cerr << "***ERROR*** (" << filename << ": " << line << ")\n";
            switch (error)
            {
            case ALC_INVALID_VALUE:
                std::cerr << "ALC_INVALID_VALUE: an invalid value was passed to an OpenAL function";
                break;
            case ALC_INVALID_DEVICE:
                std::cerr << "ALC_INVALID_DEVICE: a bad device was passed to an OpenAL function";
                break;
            case ALC_INVALID_CONTEXT:
                std::cerr << "ALC_INVALID_CONTEXT: a bad context was passed to an OpenAL function";
                break;
            case ALC_INVALID_ENUM:
                std::cerr << "ALC_INVALID_ENUM: an unknown enum value was passed to an OpenAL function";
                break;
            case ALC_OUT_OF_MEMORY:
                std::cerr << "ALC_OUT_OF_MEMORY: an unknown enum value was passed to an OpenAL function";
                break;
            default:
                std::cerr << "UNKNOWN ALC ERROR: " << error;
            }
            std::cerr << std::endl;
            return false;
        }
        return true;
	}

	bool SoundManager::get_available_devices(std::vector<std::string>& devicesVec, ALCdevice* device)
	{
        const ALCchar* devices;
        if (!alcCall(alcGetString, devices, device, nullptr, ALC_DEVICE_SPECIFIER))
            return false;

        const char* ptr = devices;

        devicesVec.clear();

        do
        {
            devicesVec.push_back(std::string(ptr));
            ptr += devicesVec.back().size() + 1;
        } while (*(ptr + 1) != '\0');

        return true;
	}

    std::int32_t FLOOF::SoundManager::convert_to_int(char* buffer, std::size_t len) {
        std::int32_t a = 0;
        if (std::endian::native == std::endian::little)
            std::memcpy(&a, buffer, len);
        else
            for (std::size_t i = 0; i < len; ++i)
                reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
        return a;
    }

    bool SoundManager::load_wav_file_header(std::ifstream& file,
	        std::uint8_t& channels,
	        std::int32_t& sampleRate,
	        std::uint8_t& bitsPerSample,
			ALsizei& size) {
        char buffer[4];
        if (!file.is_open())
            return false;

        // the RIFF
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read RIFF" << std::endl;
            return false;
        }
        if (std::strncmp(buffer, "RIFF", 4) != 0)
        {
            std::cerr << "ERROR: file is not a valid WAVE file (header doesn't begin with RIFF)" << std::endl;
            return false;
        }

        // the size of the file
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read size of file" << std::endl;
            return false;
        }

        // the WAVE
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read WAVE" << std::endl;
            return false;
        }
        if (std::strncmp(buffer, "WAVE", 4) != 0)
        {
            std::cerr << "ERROR: file is not a valid WAVE file (header doesn't contain WAVE)" << std::endl;
            return false;
        }

        // "fmt/0"
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read fmt/0" << std::endl;
            return false;
        }

        // this is always 16, the size of the fmt data chunk
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read the 16" << std::endl;
            return false;
        }

        // PCM should be 1?
        if (!file.read(buffer, 2))
        {
            std::cerr << "ERROR: could not read PCM" << std::endl;
            return false;
        }

        // the number of channels
        if (!file.read(buffer, 2))
        {
            std::cerr << "ERROR: could not read number of channels" << std::endl;
            return false;
        }
        channels = convert_to_int(buffer, 2);

        // sample rate
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read sample rate" << std::endl;
            return false;
        }
        sampleRate = convert_to_int(buffer, 4);

        // (sampleRate * bitsPerSample * channels) / 8
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read (sampleRate * bitsPerSample * channels) / 8" << std::endl;
            return false;
        }

        // ?? dafaq
        if (!file.read(buffer, 2))
        {
            std::cerr << "ERROR: could not read dafaq" << std::endl;
            return false;
        }

        // bitsPerSample
        if (!file.read(buffer, 2))
        {
            std::cerr << "ERROR: could not read bits per sample" << std::endl;
            return false;
        }
        bitsPerSample = convert_to_int(buffer, 2);

        // data chunk header "data"
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read data chunk header" << std::endl;
            return false;
        }
        if (std::strncmp(buffer, "data", 4) != 0)
        {
            std::cerr << "ERROR: file is not a valid WAVE file (doesn't have 'data' tag)" << std::endl;
            return false;
        }

        // size of data
        if (!file.read(buffer, 4))
        {
            std::cerr << "ERROR: could not read data size" << std::endl;
            return false;
        }
        size = convert_to_int(buffer, 4);

        /* cannot be at the end of file */
        if (file.eof())
        {
            std::cerr << "ERROR: reached EOF on the file" << std::endl;
            return false;
        }
        if (file.fail())
        {
            std::cerr << "ERROR: fail state set on the file" << std::endl;
            return false;
        }

        return true;
	}

    char* SoundManager::load_wav(const std::string& filename,
	        std::uint8_t& channels,
	        std::int32_t& sampleRate,
		    std::uint8_t& bitsPerSample,
	        ALsizei& size) {
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open())
        {
            std::cerr << "ERROR: Could not open \"" << filename << "\"" << std::endl;
            return nullptr;
        }
        if (!load_wav_file_header(in, channels, sampleRate, bitsPerSample, size))
        {
            std::cerr << "ERROR: Could not load wav header of \"" << filename << "\"" << std::endl;
            return nullptr;
        }

        char* data = new char[size];

        in.read(data, size);

        return data;
    }

    SoundManager2::SoundManager2()
    {
        // Find the default audio device

        // Save the default audio device as a string
        const ALCchar* defaultDeviceString = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);

        // TODO: make choosing devices possible

        // Open the default device
        ALCdevice* device = alcOpenDevice(defaultDeviceString);

        // Log if device not found
        if (!device) {
            std::cerr << "Failed to get the default device for OpenAL" << std::endl;
        	return;
        }

        // Log name of device
        std::cout << "OpenAL Device: " << alcGetString(device, ALC_DEFAULT_DEVICE_SPECIFIER) << std::endl;

        // Check for device errors
        OpenAL_ErrorCheck(device);


        // Create an OpenAL audio context from the device
        ALCcontext* context = alcCreateContext(device, nullptr);

        // Activate context so that OpenAL state modifications are applied to the context (moving sound sources etc.)
        if (!alcMakeContextCurrent(context)) {
            std::cerr << "failed to make the OpenAl context the current context" << std::endl;
            return;
        }

        OpenAL_ErrorCheck("Make context current");

        // Create a listerner in 3D space (Should be updated with players actual value later)
        // Players position
        alec(alListener3f(AL_POSITION, 0.f, 0.f, 0.f));
        // Players velocity
        alec(alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f));

        ALfloat forwardAndUpVectors[] = {
            1.f,0.f,0.f, // forward
            0.f,1.f,0.f // up
        };

        // Players orientation
        alec(alListenerfv(AL_ORIENTATION, forwardAndUpVectors));

        // Create buffers to hold the sound data

        struct ReadWavData {
            unsigned int channels = 0;
            unsigned int sampleRate = 0;
            drwav_uint64 totalPCMFrameCount = 0;
            std::vector<uint16_t> pcmData;
            drwav_uint64 getTotalSamples() { return totalPCMFrameCount * channels; } // in case of for exampe stereo files
        };
        ReadWavData monoData;
        {
            // Read PCM frames to short 16 to a heap allocated array of data
            drwav_int16* pSamleData = drwav_open_file_and_read_pcm_frames_s16("Assets/Sounds/TestSound_Stereo.wav", &monoData.channels, &monoData.sampleRate, &monoData.totalPCMFrameCount, nullptr);

            // Check if array is null
            if (pSamleData == NULL) { 
                std::cerr << "Failed to load audio file" << std::endl;
                // Free the memory allocated by drwav
                drwav_free(pSamleData, nullptr);
                return;
            }

            // Check if data read is too large for the vector
            if (monoData.getTotalSamples() > drwav_uint64(std::numeric_limits<size_t>::max())) {
	            std::cerr << "Too much data in file for 32bit addressed vector" << std::endl;
                return;
            }

            // Resize PCM data to hold all audio samples (resized to account for multiple channels by multiplying frames with channels)
            monoData.pcmData.resize(size_t(monoData.getTotalSamples()));

            // Copies the loaded samples (pSampleData) into the vector (monoData.pcmData.data())
            std::memcpy(monoData.pcmData.data(), pSamleData, monoData.pcmData.size() * 2 ); //memcpy works with bytes, and pcmData i stored with 16 bits, so there are two bytes for every stored value

            // Free the memory allocated by drwav
            drwav_free(pSamleData, nullptr);
        }

        // Pass data to OpenAL
        ALuint monoSoundBuffer;
        alec(alGenBuffers(1, &monoSoundBuffer)); // Generates buffers 
        alec(alBufferData(
            monoSoundBuffer, // The buffer
            monoData.channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, // Using the AL format Mono16 if one channel, or Stereo 16 if more
            monoData.pcmData.data(), // The data pointer
            monoData.pcmData.size() * 2, // Multiplied by two since vector is stored in 16 bits and data is read in bytes
            monoData.sampleRate)); // The sample rate, obiously

        ReadWavData stereoData;
        {
            // Read PCM frames to short 16 to a heap allocated array of data
            drwav_int16* pSamleData = drwav_open_file_and_read_pcm_frames_s16("Assets/Sounds/TestSound_Stereo.wav", &stereoData.channels, &stereoData.sampleRate, &stereoData.totalPCMFrameCount, nullptr);

            // Check if array is null
            if (pSamleData == NULL) {
                std::cerr << "Failed to load audio file" << std::endl;
                // Free the memory allocated by drwav
                drwav_free(pSamleData, nullptr);
                return;
            }

            // Check if data read is too large for the vector
            if (stereoData.getTotalSamples() > drwav_uint64(std::numeric_limits<size_t>::max())) {
                std::cerr << "Too much data in file for 32bit addressed vector" << std::endl;
                return;
            }

            // Resize PCM data to hold all audio samples (resized to account for multiple channels by multiplying frames with channels)
            stereoData.pcmData.resize(size_t(stereoData.getTotalSamples()));

            // Copies the loaded samples (pSampleData) into the vector (monoData.pcmData.data())
            std::memcpy(stereoData.pcmData.data(), pSamleData, stereoData.pcmData.size() * 2); //memcpy works with bytes, and pcmData i stored with 16 bits, so there are two bytes for every stored value

            // Free the memory allocated by drwav
            drwav_free(pSamleData, nullptr);
        }

        // Pass data to OpenAL
        ALuint stereoSoundBuffer;
        alec(alGenBuffers(1, &stereoSoundBuffer)); // Generates buffers 
        alec(alBufferData(
            stereoSoundBuffer, // The buffer
            stereoData.channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, // Using the AL format Mono16 if one channel, or Stereo 16 if more
            stereoData.pcmData.data(), // The data pointer
            stereoData.pcmData.size() * 2, // Multiplied by two since vector is stored in 16 bits and data is read in bytes
            stereoData.sampleRate)); // The sample rate, obiously

        // Create a mono sound source that plays the sound from the buffer
        // Update position for source with actualt position later
        ALuint monoSource;
        alec(alGenSources(1, &monoSource));
        // Update position of source
        alec(alSource3f(monoSource, AL_POSITION, 1.f, 0.f, 0.f));
        // Update velocity of source
        alec(alSource3f(monoSource, AL_VELOCITY, 0.f, 0.f, 0.f));
        // Update pitch of source
        alec(alSourcef(monoSource, AL_PITCH, 1.f));
        // Update pitch of source
        alec(alSourcef(monoSource, AL_GAIN, 1.f));
        alec(alSourcei(monoSource, AL_LOOPING, AL_FALSE));
        alec(alSourcei(monoSource, AL_BUFFER, monoSoundBuffer)); // The buffer integer the source should play from

        // Create a stereo sound source that plays the sound from the buffer
        // Update position for source with actualt position later
        ALuint stereoSource;
        alec(alGenSources(1, &stereoSource));
        // Update position of source -- Should not be positioned in 3D space like mono sounds
        //alec(alSource3f(stereoSource, AL_POSITION, 1.f, 0.f, 0.f));
        //// Update velocity of source
        //alec(alSource3f(stereoSource, AL_VELOCITY, 0.f, 0.f, 0.f));
        // Update pitch of source
        alec(alSourcef(stereoSource, AL_PITCH, 1.f));
        // Update pitch of source
        alec(alSourcef(stereoSource, AL_GAIN, 1.f));
        alec(alSourcei(stereoSource, AL_LOOPING, AL_FALSE));
        alec(alSourcei(stereoSource, AL_BUFFER, stereoSoundBuffer)); // The buffer integer the source should play from

        // Play the mono sound source
        alec(alSourcePlay(monoSource));
        // Query if the source is still playing
        ALint sourceState;
        alec(alGetSourcei(
            monoSource, // What source
            AL_SOURCE_STATE, // Get state
            &sourceState)) // Where to save state;

		while (sourceState == AL_PLAYING) {
	        // Loops until done playing
            alec(alGetSourcei(monoSource, AL_SOURCE_STATE, &sourceState));
        }

        // Play the stereo sound source after the mono is finished
        alec(alSourcePlay(stereoSource));
        // Query if the source is still playing
        alec(alGetSourcei(
            stereoSource, // What source
            AL_SOURCE_STATE, // Get state
            &sourceState)) // Where to save state;

            while (sourceState == AL_PLAYING) {
                // Loops until done playing
                alec(alGetSourcei(stereoSource, AL_SOURCE_STATE, &sourceState));
            }

        // Clean up resources
        // Delete sources
        alec(alDeleteSources(1, &monoSource));
        alec(alDeleteSources(1, &stereoSource));
        // Delete buffers
        alec(alDeleteBuffers(1, &monoSource));
        alec(alDeleteBuffers(1, &stereoSource));
        // Make the current context null
        alec(alcMakeContextCurrent(nullptr));
        // Destroy the current context
        alec(alcDestroyContext(context));
        alec(alcCloseDevice(device));


    }


}

