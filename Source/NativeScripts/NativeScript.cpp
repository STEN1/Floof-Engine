#include "NativeScript.h"
#include "../SoundComponent.h"
namespace FLOOF {
    void NativeScript::OnUpdate(float deltaTime) {
        if (!PlayMode) { 
            PlayMode = true;
            OnPlay();
        }

    }
    void NativeScript::EditorUpdate(float deltaTime) {
            if (PlayMode) {
            PlayMode = false;
			OnStop();
        }
        
    }
    entt::entity NativeScript::CreateEntity(const std::string& tag, entt::entity parent)
    {
        return m_Scene->CreateEntity(tag, parent);
    }
}