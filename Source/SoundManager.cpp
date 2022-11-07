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
	wavFile wavFile::ReadWav(std::string path)
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

    void NewSoundManager::SetListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 forward, glm::vec3 up) {
        alec(alListener3f(AL_POSITION, position.x, position.y, position.z));

        alec(alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z));

        ALfloat forwardAndUpVectors[] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
        alec(alListenerfv(AL_ORIENTATION, forwardAndUpVectors));
    }

    void NewSoundManager::InitOpenAL() {

    	// Find and set the default audio device
        const ALCchar* defaultDeviceString = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER); 
        s_Device = alcOpenDevice(defaultDeviceString); 

    	if (!s_Device) { std::cerr << "Failed to get the default device for OpenAL" << std::endl; return; }

        // Log name of device
        std::cout << "OpenAL Device: " << alcGetString(s_Device, ALC_DEFAULT_DEVICE_SPECIFIER) << std::endl;

        // Create an OpenAL audio context from the device
        s_Context = alcCreateContext(s_Device, nullptr);
        if (!alcMakeContextCurrent(s_Context)) { std::cerr << "failed to make the OpenAl context the current context" << std::endl; return; }

        OpenAL_ErrorCheck("Make context current");
    }

    void NewSoundManager::CleanOpenAL() {
        // Make the current context null
        alec(alcMakeContextCurrent(nullptr));
        // Destroy the current context
        alec(alcDestroyContext(s_Context));
        alec(alcCloseDevice(s_Device));
    }

    std::vector<std::string> NewSoundManager::GetAvailableDevices() {
        const ALCchar* devices;
        std::vector<std::string> devicesVec;

        // This extension provides for enumeration of the available OpenAL devices through alcGetString. An alcGetString query of ALC_DEVICE_SPECIFIER with a NULL device passed in will return a list of devices. Each device name will be separated by a single NULL character and the list will be terminated with two NULL characters.
        if (alcIsExtensionPresent(NULL, "ALC_enumeration_EXT") == AL_FALSE)
            return std::vector<std::string>(1, "COULD NOT GET DEVICES");

        if (alcIsExtensionPresent(NULL, "ALC_enumerate_all_EXT") == AL_FALSE)
            devices = (char*)alcGetString(NULL, ALC_DEVICE_SPECIFIER);
        else
            devices = (char*)alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);

        printf("List of all available input devices:\n");

        do {
            printf("  %s\n", devices);
            devicesVec.push_back(std::string(devices));
            devices += devicesVec.back().size() + 1;
        } while (*devices != '\0' && *(devices + 1) != '\0');

        return devicesVec;
    }

    void NewSoundManager::SetNewDevice(std::string device) {

        std::vector<std::string> devices = GetAvailableDevices();
        bool deviceExist{ false };
        for (auto it : devices) {
            if (it == device) { deviceExist = true; break; };
        }
        if (deviceExist) {
            // Could probably add more error checking here, like in the init function
            s_Device = alcOpenDevice(device.c_str());
            s_Context = alcCreateContext(s_Device, nullptr);
        }
        else
        {
            std::cerr << "failed to make set new OpenAl device" << std::endl;
        }
    }
    ALuint NewSoundManager::readWavData(std::string& path) {

    }

    std::vector<std::string> GetAvailableDevices()
	{
        const ALCchar* devices;
        std::vector<std::string> devicesVec;

        // This extension provides for enumeration of the available OpenAL devices through alcGetString. An alcGetString query of ALC_DEVICE_SPECIFIER with a NULL device passed in will return a list of devices. Each device name will be separated by a single NULL character and the list will be terminated with two NULL characters.
        if (alcIsExtensionPresent(NULL, "ALC_enumeration_EXT") == AL_FALSE)
            return std::vector<std::string>(1, "COULD NOT GET DEVICES");

        if (alcIsExtensionPresent(NULL, "ALC_enumerate_all_EXT") == AL_FALSE)
            devices = (char*)alcGetString(NULL, ALC_DEVICE_SPECIFIER);
        else
            devices = (char*)alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);

        printf("List of all available input devices:\n");

        do {
            printf("  %s\n", devices);
            devicesVec.push_back(std::string(devices));
            devices += devicesVec.back().size() + 1;
        } while (*devices != '\0' && *(devices + 1) != '\0');

        return devicesVec;
	}

    void PrintDevices(const ALCchar* list)
    {
        ALCchar* ptr, * nptr;

        ptr = (ALCchar*)list;
        printf("list of all available input devices:\n");
        if (!list)
        {
            printf("none\n");
        }
        else
        {
            nptr = ptr;
            while (*(nptr += strlen(ptr) + 1) != 0)
            {
                printf("  %s\n", ptr);
                ptr = nptr;
            }
            printf("  %s\n", ptr);
        }
    }




	SoundManager::SoundManager() {

    }


    void SoundManager::testSound()
    {
        std::vector<std::string> devices = GetAvailableDevices();

        readSounds();
        PlaySounds();
        //newTest();
    }

    int SoundManager::loadPath(std::string path)
    {
        int id;
        bool alreadyLoaded{ false };

	    if (!paths.empty())
	    {
		    for (int i = 0; i < paths.size(); ++i)
		    {
			    if (paths[i] == path)
			    {
                    id = i;
                    alreadyLoaded = true;
			    }   
		    }
	    }

        if (!alreadyLoaded)
        {
            paths.push_back(path);
        	id = paths.size()-1;
        }

        return id;
    }

    void SoundManager::loadAssets()
    {
	    for (auto path : paths) {
            soundData.push_back(readWavData(path.c_str()));
	    }

    }

    void SoundManager::updatePlayer(glm::vec3 pos, glm::vec3 vel, glm::vec3 forward, glm::vec3 up)
    {
        playerPosition = pos;
        playerVelocity = vel;
        playerForward = forward;
        playerUp = up;

    }

    void SoundManager::readSounds()
    {
        soundData.push_back(readWavData("Assets/Sounds/TestSound_Stereo.wav"));
        soundData.push_back(readWavData("Assets/Sounds/TestSound_Mono.wav"));
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
        //OpenAL_ErrorCheck(device);
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

    void SoundManager::PlaySounds()
    {
        openDevice();
        createContext();
        createListener();

        std::vector<ALuint> sources;

        ALuint soundBuffer;
        alec(alGenBuffers(1, &soundBuffer)); // Generates buffers 
        for (auto sound_data : soundData) {
            
            // Pass data to OpenAL

            alec(alBufferData(
                soundBuffer, // The buffer
                sound_data.channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, // Using the AL format Mono16 if one channel, or Stereo 16 if more
                sound_data.pcmData.data(), // The data pointer
                sound_data.pcmData.size() * 2, // Multiplied by two since vector is stored in 16 bits and data is read in bytes
                sound_data.sampleRate)); // The sample rate, obiously

            ALuint source;
            // Create a sound source that plays the sound from the buffer
            // Update position for source with actualt position later
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

            sources.push_back(source);
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
            // Delete sources
            alec(alDeleteSources(1, &source));
            // Delete buffers
            alec(alDeleteBuffers(1, &source));
        }

        // Make the current context null
        alec(alcMakeContextCurrent(nullptr));
        // Destroy the current context
        alec(alcDestroyContext(context));
        alec(alcCloseDevice(device));

    }

    void SoundManager::newTest()
    {
        openDevice();
        createContext();
        createListener();

        const int numSources = 2;
        const int numBuffers = 2;

        ALuint sources[2];

        ALuint soundBuffers[2];

        alec(alGenBuffers(numSources, soundBuffers)); // Generates buffers 
        alec(alGenSources(numBuffers, sources));


        // Pass data to OpenAL

        for (int i = 0; i < soundData.size(); ++i)
        {
            alec(alBufferData(soundBuffers[i], soundData[i].channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, soundData[i].pcmData.data(), soundData[0].pcmData.size() * 2, soundData[i].sampleRate));


            alec(alSource3f(sources[i], AL_POSITION, 1.f, 0.f, 0.f));
            alec(alSource3f(sources[i], AL_VELOCITY, 0.f, 0.f, 0.f));
            alec(alSourcef(sources[i], AL_PITCH, 1.f));
            alec(alSourcef(sources[i], AL_GAIN, 1.f));
            alec(alSourcei(sources[i], AL_LOOPING, AL_FALSE));
            alec(alSourcei(sources[i], AL_BUFFER, soundBuffers[i]));

        }

        for (int i = 0; i < numSources; ++i) {
            alec(alSourcePlay(sources[i]));
            alec(alSourcePlay(sources[i]));
        }

        ALint sourceState = AL_PLAYING;

            while (sourceState == AL_PLAYING) {
                for (int i = 0; i < sizeof(sources); ++i)
                {
                    alec(alGetSourcei(sources[i], AL_SOURCE_STATE, &sourceState));
                    if (sourceState == AL_PLAYING && i != sizeof(sources) - 1) {
                        break;
                    }

                }
            }

        for (int i = 0; i < sizeof(sources); ++i) {
            alec(alDeleteSources(numSources, sources));
            alec(alDeleteBuffers(numBuffers, soundBuffers));
        }

        // Make the current context null
        alec(alcMakeContextCurrent(nullptr));
        // Destroy the current context
        alec(alcDestroyContext(context));
        alec(alcCloseDevice(device));
    }

}

