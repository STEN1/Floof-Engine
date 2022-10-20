#include "Scene.h"

namespace FLOOF {
    Scene::Scene() {

    }
    entt::registry& Scene::GetCulledScene() {
        return m_Scene;
    }
    void Scene::Clear()
    {
        m_Scene.clear();
    }
}