#include "SoundManager.h"

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

	//bool get_available_devices(std::vector<std::string>& devicesVec, ALCdevice* device)
	//{
 //       const ALCchar* devices;
 //       if (!alcCall(alcGetString, devices, device, nullptr, ALC_DEVICE_SPECIFIER))
 //           return false;

 //       const char* ptr = devices;

 //       devicesVec.clear();

 //       do
 //       {
 //           devicesVec.push_back(std::string(ptr));
 //           ptr += devicesVec.back().size() + 1;
 //       } while (*(ptr + 1) != '\0');

 //       return true;
	//}


    SoundManager::SoundManager()
    {
        openDevice();
        test();

    }

    void SoundManager::updatePlayer(glm::vec3 pos, glm::vec3 vel, glm::vec3 forward, glm::vec3 up)
    {
        playerPosition = pos;
        playerVelocity = vel;
        playerForward = forward;
        playerUp = up;

    }

    void SoundManager::openDevice()
    {
        // Find the default audio device

        // Save the default audio device as a string
        const ALCchar* defaultDeviceString = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);

        // TODO: make choosing devices possible

        // Open the default device
        device = alcOpenDevice(defaultDeviceString);

        // Log if device not found
        if (!device) {
            std::cerr << "Failed to get the default device for OpenAL" << std::endl;
            return;
        }

        // Log name of device
        std::cout << "OpenAL Device: " << alcGetString(device, ALC_DEFAULT_DEVICE_SPECIFIER) << std::endl;

        // Check for device errors
        OpenAL_ErrorCheck(device);
    }

    void SoundManager::createContext()
    {
        // Create an OpenAL audio context from the device
        context = alcCreateContext(device, nullptr);

        // Activate context so that OpenAL state modifications are applied to the context (moving sound sources etc.)
        if (!alcMakeContextCurrent(context)) {
            std::cerr << "failed to make the OpenAl context the current context" << std::endl;
            return;
        }

        OpenAL_ErrorCheck("Make context current");
    }

    void SoundManager::createListener()
    {
        // Create a listerner in 3D space (Should be updated with players actual value later)
        // Players position
        alec(alListener3f(AL_POSITION, playerPosition.x, playerPosition.y, playerPosition.z));
        // Players velocity
        alec(alListener3f(AL_VELOCITY, playerVelocity.x, playerVelocity.y, playerVelocity.z));

        ALfloat forwardAndUpVectors[] = {
            playerForward.x, playerForward.y, playerForward.z, 
            playerUp.x, playerUp.y, playerUp.z 
        };

        // Players orientation
        alec(alListenerfv(AL_ORIENTATION, forwardAndUpVectors));

    }

    void SoundManager::test()
    {
        createContext();
        createListener();

        wavFile soundData = readWavData("Assets/Sounds/TestSound_Stereo.wav");

        // Pass data to OpenAL
        ALuint soundBuffer;
        alec(alGenBuffers(1, &soundBuffer)); // Generates buffers 
        alec(alBufferData(
            soundBuffer, // The buffer
            soundData.channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, // Using the AL format Mono16 if one channel, or Stereo 16 if more
            soundData.pcmData.data(), // The data pointer
            soundData.pcmData.size() * 2, // Multiplied by two since vector is stored in 16 bits and data is read in bytes
            soundData.sampleRate)); // The sample rate, obiously

        // Create a sound source that plays the sound from the buffer
        // Update position for source with actualt position later
        ALuint source;
        alec(alGenSources(1, &source));
        // Update position of source
        alec(alSource3f(source, AL_POSITION, 1.f, 0.f, 0.f));
        // Update velocity of source
        alec(alSource3f(source, AL_VELOCITY, 0.f, 0.f, 0.f));
        // Update pitch of source
        alec(alSourcef(source, AL_PITCH, 1.f));
        // Update pitch of source
        alec(alSourcef(source, AL_GAIN, 1.f));
        alec(alSourcei(source, AL_LOOPING, AL_FALSE));
        alec(alSourcei(source, AL_BUFFER, soundBuffer)); // The buffer integer the source should play from

        // Play the sound source
        alec(alSourcePlay(source));
        // Query if the source is still playing
        ALint sourceState;
        alec(alGetSourcei(
            source, // What source
            AL_SOURCE_STATE, // Get state
            &sourceState)) // Where to save state;

            while (sourceState == AL_PLAYING) {
                // Loops until done playing
                alec(alGetSourcei(source, AL_SOURCE_STATE, &sourceState));
            }

        // Clean up resources
        // Delete sources
        alec(alDeleteSources(1, &source));
        // Delete buffers
        alec(alDeleteBuffers(1, &source));
        // Make the current context null
        alec(alcMakeContextCurrent(nullptr));
        // Destroy the current context
        alec(alcDestroyContext(context));
        alec(alcCloseDevice(device));

    }

    wavFile SoundManager::readWavData(std::string path)
    {
        wavFile soundData;
        // Read PCM frames to short 16 to a heap allocated array of data
        drwav_int16* pSamleData = drwav_open_file_and_read_pcm_frames_s16(path.c_str(), &soundData.channels, &soundData.sampleRate, &soundData.totalPCMFrameCount, nullptr);

        // Check if array is null
        if (pSamleData == NULL) {
            std::cerr << "Failed to load audio file" << std::endl;
            // Free the memory allocated by drwav
            drwav_free(pSamleData, nullptr);
            return soundData;
        }

        // Check if data read is too large for the vector
        if (soundData.getTotalSamples() > drwav_uint64(std::numeric_limits<size_t>::max())) {
            std::cerr << "Too much data in file for 32bit addressed vector" << std::endl;
            return soundData;
        }

        // Resize PCM data to hold all audio samples (resized to account for multiple channels by multiplying frames with channels)
        soundData.pcmData.resize(size_t(soundData.getTotalSamples()));

        // Copies the loaded samples (pSampleData) into the vector (monoData.pcmData.data())
        std::memcpy(soundData.pcmData.data(), pSamleData, soundData.pcmData.size() * 2); //memcpy works with bytes, and pcmData i stored with 16 bits, so there are two bytes for every stored value

        // Free the memory allocated by drwav
        drwav_free(pSamleData, nullptr);
        return soundData;
    }
}

