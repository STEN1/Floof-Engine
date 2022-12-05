#include "NativeScript.h"

namespace FLOOF {
    entt::entity NativeScript::CreateEntity(const std::string& tag, entt::entity parent)
    {
        return m_Scene->CreateEntity(tag, parent);
    }
}