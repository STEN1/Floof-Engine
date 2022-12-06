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
		SoundComponent(const std::string& path);
		SoundComponent(std::initializer_list<std::string> clips);
		SoundComponent(std::vector<std::string> clips);
		~SoundComponent();

		void NewDeviceReload();

		std::shared_ptr<SoundClip> AddClip(const std::string& path);
		std::shared_ptr<SoundClip> GetClip(const std::string& name);

		void Play(SoundClip* clip);
		void Play(const std::string& name);




		bool IsPlaying(const std::string& name);

		void AddToQueue(const std::string& name);
		void PlayQueue();

		std::unordered_map<std::string, std::shared_ptr<SoundClip>> mClips;
		std::shared_ptr<SoundClip> m_SourcesPlaying; // Only here when playing



		void OnUpdate(); // Check all sources playing and remove finished

		bool PlayOnPlay{ true };
		void OnPlay();

		void CheckPlaying();
		std::queue<SoundClip*> m_PlayQueue;
		SoundClip* m_DefaultSoundClip;
		std::shared_ptr<SoundClip> m_SelectedClip;
		float m_Volume{ 1.f };

	};

}
