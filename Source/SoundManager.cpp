#include "SoundManager.h"

#define DR_WAV_IMPLEMENTATION
#include <dr_libs/dr_wav.h>
#include "Scene.h"

#include "Application.h"

namespace FLOOF {
    WavFile WavFile::ReadWav(std::string path) {
        WavFile soundData;
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

    void SoundManager::SetListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 forward, glm::vec3 up) {
        alec(alListener3f(AL_POSITION, position.x, position.y, position.z));

        alec(alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z));

        ALfloat forwardAndUpVectors[] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
        alec(alListenerfv(AL_ORIENTATION, forwardAndUpVectors));
    }

    void SoundManager::InitOpenAL(std::string device) {

        LastCamPos = glm::vec3(0.f);

        UpdateDeviceList();
        if (device != "DEFAULT") {
            bool deviceExist{ false };
            for (auto it : s_DeviceList) {
                if (it == device) { deviceExist = true; break; };
            }

            if (deviceExist) {
                s_Device = alcOpenDevice(device.c_str());
                needsReload = true;
            }
            else { device = "DEFAULT"; }
        }

        if (device == "DEFAULT") {
            const ALCchar* defaultDeviceString = alcGetString(nullptr, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
            s_Device = alcOpenDevice(defaultDeviceString);

        }

        if (!s_Device) { std::cerr << "Failed to get the default device for OpenAL" << std::endl; return; }

        // Log name of device
        std::cout << "OpenAL Device: " << alcGetString(s_Device, ALC_DEFAULT_DEVICE_SPECIFIER) << std::endl;

        // Create an OpenAL audio context from the device
        s_Context = alcCreateContext(s_Device, nullptr);
        if (!alcMakeContextCurrent(s_Context)) { std::cerr << "failed to make the OpenAl context the current context" << std::endl; return; }

        OpenAL_ErrorCheck("Make context current");

        alec(alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

        )
    }

    void SoundManager::CleanOpenAL() {

        //auto& app = Application::Get();
        //auto& registry = app.m_Scene->GetRegistry();

        //auto view = registry.view<SoundSourceComponent>();
        //for (auto [entity, soundsource] : view.each()) {
           // if (soundsource.isPlaying)
        //    {
        //        soundsource.Stop();
              //  
           // }
        //}

        // Make the current context null
        alec(alcMakeContextCurrent(nullptr));
        // Destroy the current context
        alec(alcDestroyContext(s_Context));
        alec(alcCloseDevice(s_Device));
    }

    std::vector<std::string> SoundManager::GetDeviceList() {
        return s_DeviceList;
    }

    void SoundManager::UpdateDeviceList() {
        s_DeviceList = GetAvailableDevices();
    }

    std::vector<std::string> SoundManager::GetAvailableDevices() {
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

    void SoundManager::SetNewDevice(std::string device) {

        CleanOpenAL();
        InitOpenAL(device);
    }

    void SoundManager::Update(Scene* scene, CameraComponent* camera) {


        float globalRefDistance = 125.0f;
        float globalMaxDistance = 1250.0f;

        CameraVelocity = LastCamPos - camera->Position;
        LastCamPos = camera->Position;

        float velScale{ 1.f };

        // TODO: Add listener velocity
        SetListener(camera->Position, CameraVelocity * velScale, camera->Forward, camera->Up);

        auto view = scene->GetRegistry().view<TransformComponent, SoundComponent>();
        for (auto [entity, transform, soundsource] : view.each()) {

            glm::vec3 vel{ 0.f };

            auto rigid = scene->TryGetComponent<RigidBodyComponent>(entity);
            if (rigid) { vel = rigid->GetAngularVelocity(); }

            if (needsReload == true) { soundsource.NewDeviceReload(); } // If device has changed
            auto pos = transform.GetWorldPosition();

            for (auto it = soundsource.mClips.begin(); it != soundsource.mClips.end(); it++) {
                if (!it->second->UpdateIsPlaying()) { continue; } // Only update if sound is playing
                alec(alSource3f(it->second->m_Source, AL_POSITION, pos.x, pos.y, pos.z));
                alSourcef(it->second->m_Source, AL_REFERENCE_DISTANCE, globalRefDistance);
                alSourcef(it->second->m_Source, AL_MAX_DISTANCE, globalMaxDistance);
                alSource3f(it->second->m_Source, AL_VELOCITY, vel.x * velScale, vel.y * velScale, vel.z * velScale);
            }
        }
        if (needsReload == true) { needsReload = false; }

    }

    void SoundManager::UpdateVolume() {
        if (!Muted) {
            alec(alec(alListenerf(AL_GAIN, MasterVolume)));
        }
    }

    void SoundManager::UpdateMute() {

        if (!Muted) {
            alec(alec(alListenerf(AL_GAIN, 0.f)));
            Muted = true;
        }
        else if (Muted) {
            alec(alec(alListenerf(AL_GAIN, MasterVolume)));
            Muted = false;
        }
    }


    void SoundManager::UpdateDopplerFactor() {
        // Any non-negative value 
        // 0 disables the effect;
        // 1 will not change the effect
        // A value between 0 and 1 minimize the effect
        // A value greater than 1.0 will maximize the effect
        alec(alDopplerFactor(DopplerFactor));
    }

    void SoundManager::UpdateDopplerVelocity() {
        // Any non-negative non-zero value
        // Speed of sound (setting how fast sound can move in the project
        // Should be set to the projects equivalent of the real world speed of sound (around 330 m/s)
        alec(alDopplerVelocity(DopplerVelocity));
    }

    void SoundManager::SetDopplerFactor(float factor) {
        DopplerFactor = factor;
        UpdateDopplerFactor;
    }

    void SoundManager::SetDopplerVelocity(float velocity) {
        DopplerVelocity = velocity;
        UpdateDopplerVelocity;
    }

    ALuint SoundManager::LoadWav(std::string sound) {

        std::string path = "Assets/Sounds/" + sound;
        // Check if sound already loaded
        if (auto it = s_Sounds.find(path); it != s_Sounds.end())
            return it->second;

        // Read file
        WavFile soundData = WavFile::ReadWav(path);

        // Generate buffer 
        ALuint buffer;
        alec(alGenBuffers(1, &buffer));

        alec(alBufferData(
            buffer, // The buffer
            soundData.channels > 1 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, // Using the AL format Mono16 if one channel, or Stereo 16 if more
            soundData.pcmData.data(), // The data pointer
            soundData.pcmData.size() * 2, // Multiplied by two since vector is stored in 16 bits and data is read in bytes
            soundData.sampleRate)); // The sample rate, obiously

        s_Sounds[sound] = buffer;
        return buffer;
    }

    ALuint SoundManager::GenerateSource(SoundClip* source) {
        ALuint tempSource;
        alGenSources(1, &tempSource);
        if (auto error = alGetError(); error != AL_NO_ERROR)
            std::cerr << "alGenSources error: " + std::to_string(error);

        s_Sources.push_back(source);
        return tempSource;
    }

    void SoundManager::DeleteSource(SoundClip* source) {

        if (auto it = std::find(s_Sources.begin(), s_Sources.end(), source); it != s_Sources.end()) {
            alDeleteSources(1, &(*it)->m_Source);
            s_Sources.erase(it);
        }
    }

    std::vector<std::string> GetAvailableDevices() {
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

    void PrintDevices(const ALCchar* list) {
        ALCchar* ptr, * nptr;

        ptr = (ALCchar*)list;
        printf("list of all available input devices:\n");
        if (!list) {
            printf("none\n");
        }
        else {
            nptr = ptr;
            while (*(nptr += strlen(ptr) + 1) != 0) {
                printf("  %s\n", ptr);
                ptr = nptr;
            }
            printf("  %s\n", ptr);
        }
    }
}


