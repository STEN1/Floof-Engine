#ifndef FLOOF_MONSTERTRUCKSCRIPT_H
#define FLOOF_MONSTERTRUCKSCRIPT_H

#include "../CarBaseScript.h"
namespace FLOOF{
    class MonsterTruckScript : public CarBaseScript{
    public:
        MonsterTruckScript(glm::vec3 spawnLocation): CarBaseScript(spawnLocation){};
        virtual void OnCreate(Scene* scene, entt::entity entity) override;

        virtual void OnUpdate(float deltaTime) override;

        virtual void LastUpdate(float deltaTime) override;

    private:

    };
}



#endif //FLOOF_MONSTERTRUCKSCRIPT_H
