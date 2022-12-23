#ifndef FLOOF_SERVERLAYER_H
#define FLOOF_SERVERLAYER_H

#include "ApplicationLayer.h"
#include "entt/entt.hpp"
#include <unordered_map>
#include <string>
#include "Renderer/SceneRenderer.h"
#include "Scene.h"
namespace FLOOF{
    class ServerLayer : public ApplicationLayer {
    public:
        ServerLayer();
        ~ServerLayer();
        virtual void OnUpdate(double deltaTime) override;
        virtual void OnImGuiUpdate(double deltaTime) override;
        virtual VkSemaphore OnDraw(double deltaTime, VkSemaphore waitSemaphore) override;
        Scene* GetScene() { return m_Scene.get(); }
        void StartPlay();
        void StopPlay();
        bool IsPlaying() { return m_PlayModeActive; }

    private:
        void MakeServerScene();

        std::unique_ptr<Scene> m_Scene;
        bool m_PlayModeActive = true;

    };
}



#endif //FLOOF_SERVERLAYER_H
