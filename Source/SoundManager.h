#pragma once
#include "al.h"
#include "alc.h"

#include <cstring>
#include <iostream>
#include <fstream>
#include <type_traits>
#include <vector>
#include <bit>
#include <glm/vec3.hpp>

#include "dr_libs/dr_wav.h"

namespace FLOOF {
	// Buffers to hold the sound data
	struct wavFile {
		unsigned int channels{ 0 };
		unsigned int sampleRate{ 0 };
		drwav_uint64 totalPCMFrameCount{ 0 };
		std::vector<uint16_t> pcmData;
		drwav_uint64 getTotalSamples() { return totalPCMFrameCount * channels; } // in case of for exampe stereo files
	};


	// This class contains all audio code at the moment.
	// The plan is to move a lot into an audio component
	// This is just to get it up and running for the time being
	class SoundManager {
		public:
			SoundManager();
			void testSound();
			int loadPath(std::string path);
			void loadAssets();
			void updatePlayer(glm::vec3 pos, glm::vec3 vel, glm::vec3 forward, glm::vec3 up); // Every tick
		private:
			void readSounds();
			void openDevice();
			void createContext();
			void createListener();
			void PlaySounds();

			ALCdevice* device;
			ALCcontext* context;
			glm::vec3 playerPosition{ 0.f,0.f,0.f };
			glm::vec3 playerVelocity{ 0.f,0.f,0.f };
			glm::vec3 playerForward{ 1.f,0.f,0.f };
			glm::vec3 playerUp{ 0.f,1.f,0.f };
			std::vector<wavFile> soundData;
			wavFile readWavData(std::string path);

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
 * soundmanager.PlaySounds(std::vector<SoundComponent*> sounds);
 *
 * in soundmanager 

 */