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

    void SoundSourceComponent::SoundClip::Volume() {
        alec(alSourcef(m_Source, AL_GAIN, m_Volume));
    }

    void SoundSourceComponent::SoundClip::Volume(float volume) {
        m_Volume = volume;
        alec(alSourcef(m_Source, AL_GAIN, volume));
    }

    void SoundSourceComponent::SoundClip::Pitch() {
        alec(alSourcef(m_Source, AL_PITCH, m_Pitch));
    }

    void SoundSourceComponent::SoundClip::UpdateStatus() {

        ALint sourceState;
        alec(alGetSourcei(m_Source, AL_SOURCE_STATE, &sourceState))

            if (sourceState == AL_PLAYING) {
                isPlaying = true;
            }
            else {
                isPlaying = false;
            }
    }

    void SoundSourceComponent::SoundClip::Play() {
        isPlaying = true;
        alec(alSourcePlay(m_Source));
    }

    void SoundSourceComponent::SoundClip::Stop() {
        isPlaying = false;
        alec(alSourceStop(m_Source));

    }

    void SoundSourceComponent::SoundClip::Looping(bool looping) {
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
	}

	SoundComponent::SoundComponent() {
	}

	void SoundComponent::Play(SoundClip* clip) {
        clip->Play();
        m_SourcesPlaying.push_back(clip);
        if (m_DefaultSoundClip = nullptr){
            m_DefaultSoundClip = clip;
        }
	}

	void SoundComponent::OnUpdate() {
		CheckPlaying();
	}

	void SoundComponent::OnPlay() {
		if (PlayOnPlay){
            Play(m_DefaultSoundClip);
		}
	}

	void SoundComponent::CheckPlaying() {
        for (auto it = m_SourcesPlaying.begin(); it != m_SourcesPlaying.end();) {
            ALint sourceState;
            alec(alGetSourcei((*it)->m_Source, AL_SOURCE_STATE, &sourceState))

            if (sourceState != AL_PLAYING)
                m_SourcesPlaying.erase(it);
            
            else ++it;
		}
	}

	void SoundComponent::NewDeviceReload() {
        for (auto it = m_SourcesPlaying.begin(); it != m_SourcesPlaying.end(); it++)
            (*it)->NewDeviceReload();
	}
}

