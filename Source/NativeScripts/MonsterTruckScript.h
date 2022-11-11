
#ifndef FLOOF_MONSTERTRUCKSCRIPT_H
#define FLOOF_MONSTERTRUCKSCRIPT_H

#include "NativeScript.h"
namespace FLOOF {
    class MonsterTruckScript : public NativeScript {
    public:
        virtual void OnCreate(std::shared_ptr<Scene> scene, entt::entity entity) override;
        virtual void OnUpdate(float deltaTime) override;

    private:

        entt::entity frame;

        entt::entity Wheel_fr;
        entt::entity Wheel_fl;
        entt::entity Wheel_br;
        entt::entity Wheel_bl;

    };
}

#endif //FLOOF_MONSTERTRUCKSCRIPT_H
