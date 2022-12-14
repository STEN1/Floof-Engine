#pragma once

#include <cstring>
#include <iostream>
#include <fstream>
#include <type_traits>
#include <vector>
#include <bit>
#include <unordered_map>
#include <glm/vec3.hpp>

#include "al.h"
#include "alc.h"
#include "Components.h"
#include "SoundComponent.h"
#include "dr_libs/dr_wav.h"

// Check for errors
#define OpenAL_ErrorCheck(message) {\
    ALenum error = alGetError();\
    if (error != AL_NO_ERROR) std::cerr << "OpenAL Error: " << error << " with call for " << #message << std::endl;\
}

// Macro to route function calls into to check for errors
#define alec(FUNCTION_CALL)\
    FUNCTION_CALL;\
    OpenAL_ErrorCheck(FUNCTION_CALL)

namespace FLOOF {
	// Buffers to hold the sound data
	struct WavFile {
		static WavFile ReadWav(std::string path);
		unsigned int channels{ 0 };
		unsigned int sampleRate{ 0 };
		drwav_uint64 totalPCMFrameCount{ 0 };
		std::vector<uint16_t> pcmData;
		drwav_uint64 getTotalSamples() { return totalPCMFrameCount * channels; } // in case of for exampe stereo files
	};

	class SoundManager {
		friend class SoundSourceComponent;
		friend class SoundComponent;
		friend class SoundClip;
	public:
		static void SetListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 forward, glm::vec3 up);
		static void InitOpenAL(std::string device = "DEFAULT");
		static void CleanOpenAL();
		static std::vector<std::string> GetDeviceList();
		static void UpdateDeviceList();
		static void SetNewDevice(std::string device);

		static void Update(Scene* scene, CameraComponent* camera);
		static void UpdateVolume();
		static void UpdateMute();
		inline static float MasterVolume{ 1.f };
		inline static bool Muted{ false };

	private:
		static std::vector<std::string> GetAvailableDevices();
		static ALuint LoadWav(std::string sound);
		static ALuint GenerateSource(SoundClip* clip);
		static void DeleteSource(SoundClip* source);
		inline static std::vector<std::string> s_DeviceList;
		inline static std::unordered_map<std::string, ALuint> s_Sounds;
		inline static std::vector<SoundClip*> s_Sources;
		inline static ALCdevice* s_Device{ nullptr };
		inline static ALCcontext* s_Context{ nullptr };
		inline static bool needsReload{ false };

		// factory function return uniquie pointer to clips
	};
}