#include "EnvironmentSoundScript.h"
#include "../Input.h"
#include "CarBaseScript.h"

namespace FLOOF {

	EnvironmentSoundScript::EnvironmentSoundScript() {}

	EnvironmentSoundScript::~EnvironmentSoundScript() {}

	void EnvironmentSoundScript::OnCreate(Scene* scene, entt::entity entity) {
		NativeScript::OnCreate(scene, entity);
		{

			// Background ambience
			{

				ambience = scene->CreateEntity("Ambience", entity);
				scene->AddComponent<SoundComponent>(ambience, "wind.wav");
				scene->GetComponent<SoundComponent>(ambience).GetClip("wind.wav")->Looping(true);
				
				crowd = scene->CreateEntity("Crowd", entity);
				scene->AddComponent<SoundComponent>(crowd, "crowd.wav");
				scene->GetComponent<SoundComponent>(crowd).GetClip("crowd.wav")->Looping(true);
				
				radio = scene->CreateEntity("Radio", entity);
				scene->AddComponent<SoundComponent>(radio);
				scene->GetComponent<SoundComponent>(radio).AddQueues(3);
				scene->GetComponent<SoundComponent>(radio).AddToQueue(1, "pinchcliffe.wav");
				scene->GetComponent<SoundComponent>(radio).AddToQueue(1, "bardcore.wav");
				scene->GetComponent<SoundComponent>(radio).AddToQueue(2, "Danheim_Atgeir.wav");
				scene->GetComponent<SoundComponent>(radio).AddToQueue(2, "Danheim_Tyrfing.wav");
				scene->GetComponent<SoundComponent>(radio).AddToQueue(3, "Jeremy_Njol.wav");
				scene->GetComponent<SoundComponent>(radio).AddToQueue(3, "Jeremy_Utanlands.wav");
				
				radioFX = scene->CreateEntity("Radio FX", entity);
				scene->AddComponent<SoundComponent>(radioFX, "radionoise.wav");
				scene->GetComponent<SoundComponent>(radioFX).AddClip("radiobutton.wav");
				scene->GetComponent<SoundComponent>(radioFX).AddClip("click.wav");

				// Volume adjustments
				
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radionoise.wav")->Volume(1.0f);
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radiobutton.wav")->Volume(1.0f);
				scene->GetComponent<SoundComponent>(radioFX).GetClip("click.wav")->Volume(1.0f);

				scene->GetComponent<SoundComponent>(crowd).GetClip("crowd.wav")->Volume(0.4f);
				scene->GetComponent<SoundComponent>(ambience).GetClip("wind.wav")->Volume(0.4f);

				scene->GetComponent<SoundComponent>(radio).GetClip("pinchcliffe.wav")->Volume(0.2f);
				scene->GetComponent<SoundComponent>(radio).GetClip("bardcore.wav")->Volume(0.2f);
				scene->GetComponent<SoundComponent>(radio).GetClip("Danheim_Atgeir.wav")->Volume(0.2f);
				scene->GetComponent<SoundComponent>(radio).GetClip("Danheim_Tyrfing.wav")->Volume(0.2f);
				scene->GetComponent<SoundComponent>(radio).GetClip("Jeremy_Njol.wav")->Volume(0.2f);
				scene->GetComponent<SoundComponent>(radio).GetClip("Jeremy_Utanlands.wav")->Volume(0.2f);


			}
		}
	
	}

	void EnvironmentSoundScript::OnUpdate(float deltaTime) {
		NativeScript::OnUpdate(deltaTime);
 
		auto* scene = m_Scene;

		bool windowIsActive = scene->IsActiveScene();

		if (Input::Key(ImGuiKey_0) && windowIsActive) {
			if (RadioChannel != 0) {
				RadioChannel = 0;
				for (auto& clip : scene->GetComponent<SoundComponent>(radio).mClips) { clip.second->Stop(); }
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radiobutton.wav")->Play();
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radionoise.wav")->Play();

			}
		}

		if (Input::Key(ImGuiKey_1) && windowIsActive) {
			if (RadioChannel != 1) {
				RadioChannel = 1;
				for (auto& clip : scene->GetComponent<SoundComponent>(radio).mClips) { clip.second->Stop(); }
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radiobutton.wav")->Play();
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radionoise.wav")->Play();
				scene->GetComponent<SoundComponent>(radio).PlayQueue(1);

			}

		}

		if (Input::Key(ImGuiKey_2) && windowIsActive) {
			if (RadioChannel != 2) {
				RadioChannel = 2;
				for (auto& clip : scene->GetComponent<SoundComponent>(radio).mClips) { clip.second->Stop(); }
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radiobutton.wav")->Play();
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radionoise.wav")->Play();
				scene->GetComponent<SoundComponent>(radio).PlayQueue(2);
			}
		}

		if (Input::Key(ImGuiKey_3) && windowIsActive) {
			if (RadioChannel != 3) {
				RadioChannel = 3;
				for (auto& clip : scene->GetComponent<SoundComponent>(radio).mClips) { clip.second->Stop(); }
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radiobutton.wav")->Play();
				scene->GetComponent<SoundComponent>(radioFX).GetClip("radionoise.wav")->Play();
				scene->GetComponent<SoundComponent>(radio).PlayQueue(3);
			}
		}
		if (Input::Key(ImGuiKey_MouseWheelY) && windowIsActive) {
			
			float adjustment = ImGui::GetIO().MouseWheel / 30.f;
			float MaxVolume = 1.f;
			float volume = scene->GetComponent<SoundComponent>(radio).m_Volume;
			volume += adjustment;

			if (volume < 0.f) { volume = 0.f; }
			else if (volume > 1.f) { volume = 1.f; }
			else { scene->GetComponent<SoundComponent>(radioFX).GetClip("click.wav")->Play(); }

			for (auto& clip : scene->GetComponent<SoundComponent>(radio).mClips) {
				clip.second->Volume(volume);
			}

			scene->GetComponent<SoundComponent>(radio).m_Volume = volume;

		}

	}

	void EnvironmentSoundScript::EditorUpdate(float deltaTime) {
		NativeScript::EditorUpdate(deltaTime);

	}

	void EnvironmentSoundScript::OnPlay() {
		RadioChannel = 0;
		m_Scene->GetComponent<SoundComponent>(crowd).Play("crowd.wav");
		m_Scene->GetComponent<SoundComponent>(ambience).Play("wind.wav");
	}

	void EnvironmentSoundScript::OnStop() {

		// Stop all sounds in this script from playing
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


	void EnvironmentSoundScript::TerrainCollisionCallback::onBeginOverlap(void* obj1, void* obj2) {
		if (RollingSound) {
			RollingSound->Play();
			muted = false;
		}
	}

	void EnvironmentSoundScript::TerrainCollisionCallback::onOverlap(void* obj1, void* obj2) {
		CollisionDispatcher::onOverlap(obj1, obj2);

        auto view = mScene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
        for (auto [ent, player, script]: view.each()) {
            if (player.mPlayer == mScene->ActivePlayer) {
                auto checkpoint = mScene->TryGetComponent<NativeScriptComponent>(ent);
                if (checkpoint) {
                    auto *car = dynamic_cast<CarBaseScript *>(checkpoint->Script.get());
                    if (car) {

						auto & rigid = mScene->GetComponent<RigidBodyComponent>(car->frame);
						auto velocity = glm::length(rigid.GetLinearVelocity());

						if (velocity < 2.f && !muted) {
							RollingSound->Stop();
							muted = true;
						}

						else if (velocity > 2.f && muted) {
							RollingSound->Play();
							muted = false;
						}
                    }
                }
            }
        }

	}

	void EnvironmentSoundScript::TerrainCollisionCallback::onEndOverlap(void* obj) {
		CollisionDispatcher::onEndOverlap(obj);
		if (RollingSound) {
			RollingSound->Stop();
			muted = true;
		}
	}

}