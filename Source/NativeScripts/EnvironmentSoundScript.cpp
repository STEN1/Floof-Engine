#include "EnvironmentSoundScript.h"
#include "../Input.h"


namespace FLOOF {

	EnviromentSoundScript::EnviromentSoundScript() {}

	EnviromentSoundScript::~EnviromentSoundScript() {}

	void EnviromentSoundScript::OnCreate(Scene* scene, entt::entity entity) {
		NativeScript::OnCreate(scene, entity);
		{

			// Background ambience
			{

				auto ent = scene->CreateEntity("Ambience", entity);
				ambience = scene->AddComponent<SoundComponent>(ent, "wind.wav");

				auto clip = ambience.GetClip("wind.wav");
				clip->Looping(true);

				ent = scene->CreateEntity("Crowd", entity);
				crowd = scene->AddComponent<SoundComponent>(ent, "crowd.wav");

				clip = crowd.GetClip("crowd.wav");
				clip->Looping(true);

				ent = scene->CreateEntity("Radio", entity);
				radio = scene->AddComponent<SoundComponent>(ent, "pinchcliffe.wav");

				clip = radio.GetClip("pinchcliffe.wav");
				clip->Looping(true);

				clip = radio.AddClip("hum.wav");

				ent = scene->CreateEntity("Radio FX", entity);
				radioFX = scene->AddComponent<SoundComponent>(ent, "radionoise.wav");
				clip = radioFX.AddClip("radiobutton.wav");
				clip = radioFX.AddClip("click.wav");

			}
		}
	
	}

	void EnviromentSoundScript::OnUpdate(float deltaTime) {
		NativeScript::OnUpdate(deltaTime);



		if (!PlayMode) {
			PlayMode = true;
			//radio.Play("pinchcliffe.wav");
			RadioChannel = 0;
			crowd.Play("crowd.wav");
			ambience.Play("wind.wav");
		}
 
		auto* scene = m_Scene;

		bool windowIsActive = scene->IsActiveScene();

		if (Input::Key(ImGuiKey_0) && windowIsActive) {
			if (RadioChannel != 0) {
				RadioChannel = 0;
				for (auto& clip : radio.mClips) { clip.second->Stop(); }
				radioFX.GetClip("radiobutton.wav")->Play();
				radioFX.GetClip("radionoise.wav")->Play();
			}
		}

		if (Input::Key(ImGuiKey_1) && windowIsActive) {
			if (RadioChannel != 1) {
				RadioChannel = 1;
				for (auto& clip : radio.mClips) { clip.second->Stop(); }
				radioFX.GetClip("radiobutton.wav")->Play();
				radioFX.GetClip("radionoise.wav")->Play();
				radio.GetClip("pinchcliffe.wav")->Play();

			}

		}

		if (Input::Key(ImGuiKey_2) && windowIsActive) {
			if (RadioChannel != 2) {
				RadioChannel = 2;
				for (auto& clip : radio.mClips) { clip.second->Stop(); }
				radioFX.GetClip("radiobutton.wav")->Play();
				radioFX.GetClip("radionoise.wav")->Play();
				radio.GetClip("hum.wav")->Play();
			}
		}
		if (Input::Key(ImGuiKey_MouseWheelY) && windowIsActive) {
			
			float adjustment = ImGui::GetIO().MouseWheel / 30.f;
			float MaxVolume = 1.f;
			float volume = radio.m_Volume;
			volume += adjustment;

			if (volume < 0.f) { volume = 0.f; }
			else if (volume > 1.f) { volume = 1.f; }
			else { radioFX.GetClip("click.wav")->Play(); }

			for (auto& clip : radio.mClips) { 
				clip.second->Volume(volume);
			}

			radio.m_Volume = volume;

		}

	}

	void EnviromentSoundScript::EditorUpdate(float deltaTime) {
	

		// Stop all sounds in this script from playing
		if (PlayMode) {
			PlayMode = false;
			auto rel = m_Scene->TryGetComponent<Relationship>(m_Entity);

			if (rel) {
				for (auto& childEntity : rel->Children) {
					auto& childSound = m_Scene->GetComponent<SoundComponent>(childEntity);
					for (auto& clip : childSound.mClips) {
						clip.second->Stop();
					}
				}
			}
		}

	}


}