#ifndef FLOOF_CHECKPOINTSCRIPT_H
#define FLOOF_CHECKPOINTSCRIPT_H

#include "NativeScript.h"
#include "../CollisionDispatcher.h"
#include "../SoundComponent.h"

namespace FLOOF {
    class CheckPointScript : public NativeScript {
    public:
        void OnCreate(Scene *scene, entt::entity entity) override;

        void OnUpdate(float deltaTime) override;

        void SetActive(bool active);
    private:
        entt::entity Trigger;

        entt::entity Pole;
    public:

        class CheckPointCollision : public CollisionDispatcher {
        public:
            CheckPointCollision(Scene *scene, entt::entity &entity) : CollisionDispatcher(scene, entity) {};

            void onBeginOverlap(void *obj1, void *obj2) override;

            void onOverlap(void *obj1, void *obj2) override;

            void onEndOverlap(void *obj) override;

            bool IsActive{false};

            std::shared_ptr<SoundClip> ImpactSound;
            void SetImpactSound(std::shared_ptr<SoundClip> impact) { ImpactSound = impact; };
        };
    private:
        std::shared_ptr<CheckPointCollision> mCheckPointCollision;
    public:
        bool ActiveCheckPoint{false};
    };
}


#endif //FLOOF_CHECKPOINTSCRIPT_H
