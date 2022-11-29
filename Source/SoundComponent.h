#pragma once
#include <memory>
#include <string>
#include <vector>
#include "al.h"

namespace FLOOF {

	class SoundClip {
	public:
		SoundClip(const std::string& name);

		ALuint m_SourceID;
		ALuint m_SoundID;

		// + Volume, pitch 

	};

	class SoundComponent {
	public:
		void Play(SoundClip* clip);

		void Update(); // Check all sources playing and remove finished

		std::vector<ALuint> sourcesPlaying; // Only here when playing
		std::shared_ptr<SoundClip> m_DefaultSoundClip;
		bool PlayOnPlay{ true };

	};

}
