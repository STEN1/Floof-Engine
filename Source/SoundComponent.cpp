#include "SoundComponent.h"


namespace FLOOF {
	SoundClip::SoundClip(const std::string& name) {
        m_Name = name;
        m_Sound = SoundManager::LoadWav(name);
        m_Path = name;
        m_Source = SoundManager::GenerateSource(this);
        alec(alSourcei(m_Source, AL_BUFFER, m_Sound));
	}

	SoundClip::~SoundClip() {
        SoundManager::DeleteSource(this);
	}

    void SoundClip::Volume() {
        alec(alSourcef(m_Source, AL_GAIN, m_Volume));
    }

    void SoundClip::Volume(float volume) {
        m_Volume = volume;
        Volume();
    }

    void SoundClip::Pitch() {
        alec(alSourcef(m_Source, AL_PITCH, m_Pitch));
    }

    void SoundClip::Pitch(float pitch) {
        m_Pitch = pitch;
        Pitch();
    }

    bool SoundClip::UpdateIsPlaying() {

        ALint sourceState;
        alec(alGetSourcei(m_Source, AL_SOURCE_STATE, &sourceState));
    
        if (isPlaying && sourceState == AL_STOPPED) {
            isPlaying = false;
        }
               
        else if (!isPlaying && sourceState == AL_PLAYING) {
            isPlaying = true;
        }
        return isPlaying;
    }

    void SoundClip::Play() {
        isPlaying = true;
        alec(alSourcePlay(m_Source));
    }

    void SoundClip::Stop() {
        isPlaying = false;
        alec(alSourceStop(m_Source));

    }

    void SoundClip::Looping(bool looping) {
        if (looping) {
            alec(alSourcei(m_Source, AL_LOOPING, AL_TRUE));
            isLooping = true;
        }
        else {
            alec(alSourcei(m_Source, AL_LOOPING, AL_FALSE));
            isLooping = false;
        }
    }

    void SoundClip::OnUpdate() {
        if (SoundManager::needsReload == true) { NewDeviceReload(); } // If device

    }

	void SoundClip::NewDeviceReload() {
        // TODO Make sure previous source and buffer is deleted
        m_Sound = SoundManager::LoadWav(m_Name);
        m_Source = SoundManager::GenerateSource(this);
        alec(alSourcei(m_Source, AL_BUFFER, m_Sound));
        
        Looping(isLooping);
		Volume(m_Volume);
		Pitch(m_Pitch);
        if (isPlaying) { Play(); }
        
	}

	SoundComponent::SoundComponent() {
	}

	SoundComponent::SoundComponent(const std::string& path){
        AddClip(path);
	}

	SoundComponent::SoundComponent(std::initializer_list<std::string> clips) {
        for (auto clip : clips) {
            AddClip(clip);
        }
	}

	SoundComponent::SoundComponent(std::vector<std::string> clips) {
        for (auto clip : clips) {
            AddClip(clip);
        }
	}

	SoundComponent::~SoundComponent() {
	}

	void SoundComponent::NewDeviceReload() {
        for (auto it = mClips.begin(); it != mClips.end(); it++)
            (*it).second->NewDeviceReload();
    }

	std::shared_ptr<SoundClip> SoundComponent::AddClip(const std::string& path) {

        auto it = mClips.find(path);

        if (it != mClips.end()) {
            return it->second;
        }
        else {
            if (m_DefaultSoundClip.empty()) { m_DefaultSoundClip = path; }
            mClips.insert(std::pair<std::string, std::shared_ptr<SoundClip>>(path, std::make_shared<SoundClip>(path)));
            return mClips[path];
        }
		
	}

    std::shared_ptr<SoundClip> SoundComponent::GetClip(const std::string& name) {
        auto it = mClips.find(name);

        if (it != mClips.end()) {
            return mClips[name];
        }
        else {
            return nullptr;
        }
    }

    bool SoundComponent::Play(const std::string& name) {
        auto it = mClips.find(name);

        if (it != mClips.end()) {
            it->second->Play();
            return true;
        }
        else {
            return false;
        }
    }

    bool SoundComponent::Stop(const std::string& name) {
        auto it = mClips.find(name);

        if (it != mClips.end()) {
            it->second->Stop();
            return true;
        }
        else {
            return false;
        }
    }

    bool SoundComponent::SetDefaultClip(const std::string& name) {

        auto it = mClips.find(name);

        if (it != mClips.end()) {
            m_DefaultSoundClip = name;
            return true;
        }
        else {
            return false;
        }
    }

	void SoundComponent::OnPlay() {
		if (PlayOnPlay){
            Play(m_DefaultSoundClip);
		}
	}
	
}

