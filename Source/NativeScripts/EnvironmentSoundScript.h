#pragma once

#include "NativeScript.h"
#include "../Components.h"
#include "../SoundComponent.h"
#include "../CollisionDispatcher.h"

namespace FLOOF {
	class EnvironmentSoundScript : public NativeScript {
	public:
		EnvironmentSoundScript();
		
		virtual ~EnvironmentSoundScript();
		
		void OnCreate(Scene* scene, entt::entity entity) override;

		void OnUpdate(float deltaTime) override;
		
		void EditorUpdate(float deltaTime) override;

		bool PlayMode{ false };

		int RadioChannel{ 1 };

	protected:
		SoundComponent radio;
		SoundComponent ambience;
		SoundComponent crowd;
		SoundComponent radioFX;

		//class RadioChannel

		//void ChangeRadioChannel(int channel);

	public:
		class TerrainCollisionCallback : public CollisionDispatcher {
		public:
			TerrainCollisionCallback(Scene* scene, entt::entity& entity) : CollisionDispatcher(scene, entity) {};

			virtual void onBeginOverlap(void* obj1, void* obj2) override;

			virtual void onOverlap(void* obj1, void* obj2) override;

			virtual void onEndOverlap(void* obj) override;

			std::shared_ptr<SoundClip> RollingSound;
			void SetSound(std::shared_ptr<SoundClip> impact) { RollingSound = impact; };
			bool muted{ false };
		};

		std::shared_ptr<TerrainCollisionCallback> TruckCallback;

	};
}