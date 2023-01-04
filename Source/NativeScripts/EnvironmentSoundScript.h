#pragma once

#include "NativeScript.h"
#include "../Components.h"
#include "../SoundComponent.h"

namespace FLOOF {
	class EnviromentSoundScript : public NativeScript {
	public:
		EnviromentSoundScript();
		
		virtual ~EnviromentSoundScript();
		
		void OnCreate(Scene* scene, entt::entity entity) override;

		void OnUpdate(float deltaTime) override;
		
		void EditorUpdate(float deltaTime) override;

		bool PlayMode{ false };

		int RadioChannel{ 1 };

	protected:
		SoundComponent radio;
		SoundComponent ambience;
		SoundComponent crowd;

	};
}