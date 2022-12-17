#ifndef FLOOF_RACECARSCRIPT_H
#define FLOOF_RACECARSCRIPT_H


#include "../CarBaseScript.h"
namespace FLOOF{
    class RaceCarScript : public CarBaseScript{
    public:
        RaceCarScript(glm::vec3 spawnLocation): CarBaseScript(spawnLocation){};
        virtual void OnCreate(Scene* scene, entt::entity entity) override;

        virtual void OnUpdate(float deltaTime) override;

        virtual void LastUpdate(float deltaTime) override;

    private:

    };
}


#endif //FLOOF_RACECARSCRIPT_H
