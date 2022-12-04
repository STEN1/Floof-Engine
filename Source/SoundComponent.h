#pragma once
#include <memory>
#include <string>
#include <vector>
#include <queue>
#include "al.h"
#include "SoundManager.h"

namespace FLOOF {

	class SoundClip {
		friend class SoundManager;
		friend class SoundComponent;

	public:
		SoundClip(const std::string& name);
		~SoundClip();

		ALuint m_Source;
		ALuint m_Sound;

		std::string m_Path;
		std::string m_Name;
		float m_Volume{ 1.f };
		float m_Pitch{ 1.f };
		bool m_Looping{ false };

		void Volume();
		void Volume(float volume);
		void Pitch();
		void UpdateStatus();
		void Stop();
		void Looping(bool looping);

		void OnUpdate(); // Check if device reloaded
		void NewDeviceReload();

	private:
		void Play(); // Can only play when attached to a sound component

	};

	class SoundComponent {
		friend class SoundManager;
	public:
		SoundComponent();

		void Play(SoundClip* clip);
		void PlayQueue();
		void AddToQueue(SoundClip* clip);
		void OnUpdate(); // Check all sources playing and remove finished
		void OnPlay();
		void CheckPlaying();
		void NewDeviceReload();
		std::vector<SoundClip*> m_SourcesPlaying; // Only here when playing
		std::queue<SoundClip*> m_PlayQueue;
		SoundClip* m_DefaultSoundClip;
		bool PlayOnPlay{ true };

	};

}
