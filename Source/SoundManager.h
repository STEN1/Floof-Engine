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
	public:
		static void SetListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 forward, glm::vec3 up);
		static void InitOpenAL();
		static void CleanOpenAL();
		static std::vector<std::string> GetAvailableDevices();
		static void SetNewDevice(std::string device);

		static void Update();
	private:
		static ALuint LoadWav(std::string sound);
		static ALuint GenerateSource(SoundSourceComponent* source);
		static void DeleteSource(SoundSourceComponent* source);

		inline static std::unordered_map<std::string, ALuint> s_Sounds;
		inline static std::vector<SoundSourceComponent*> s_Sources;
		inline static ALCdevice* s_Device{ nullptr };
		inline static ALCcontext* s_Context{ nullptr };
		

	};

	// This class contains all audio code at the moment.
	// The plan is to move a lot into an audio component
	// This is just to get it up and running for the time being
	class OldSoundManager {
		public:
			OldSoundManager();
			void testSound();
			int loadPath(std::string path); // Could contain the whole component instead
			void loadAssets();
			void updatePlayer(glm::vec3 pos, glm::vec3 vel, glm::vec3 forward, glm::vec3 up); // Every tick
			void PlaySounds();
			void newTest();
		private:
			void readSounds();
			void openDevice();
			void createContext();
			void createListener();
			std::vector<std::string> devices;
			ALCdevice* device;
			ALCcontext* context;
			glm::vec3 playerPosition{ 0.f,0.f,0.f };
			glm::vec3 playerVelocity{ 0.f,0.f,0.f };
			glm::vec3 playerForward{ 1.f,0.f,0.f };
			glm::vec3 playerUp{ 0.f,1.f,0.f };
			std::vector<WavFile> soundData;

			std::vector<std::string> paths;
	};

	

}


/*
 * in run:
 *
 * iterate through soundcomponents
 * for every:
 * component.getID(soundmanager.loadPath(component.getPath));
 *
 * in draw
 * iterate through soundcomponents
 * save in vector of pointers and send to soundmanager
 * soundmanager.PlaySounds(std::vector<SoundSourceComponent*> sounds);
 *
 * in soundmanager 

 */