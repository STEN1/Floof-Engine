
#ifndef FLOOF_MONSTERTRUCKSCRIPT_H
#define FLOOF_MONSTERTRUCKSCRIPT_H

#include "NativeScript.h"
#include "../Components.h"

namespace FLOOF {

    class MonsterTruckScript : public NativeScript {
    public:
        virtual void OnCreate(Scene* scene, entt::entity entity) override;
        virtual void OnUpdate(float deltaTime) override;

        void CameraUi();
    private:

        entt::entity frame;

        entt::entity CamTarget;
        entt::entity Camera;

        entt::entity Wheel_fr;
        entt::entity Wheel_fl;
        entt::entity Wheel_br;
        entt::entity Wheel_bl;

        entt::entity BreakLight;
        entt::entity HeadLightR;
        entt::entity HeadLightL;

        std::vector<btHinge2Constraint*> axles;
        int graphnumb = 0;

        struct CamLocation{
            CamLocation(std::string n, glm::vec3 loc, glm::vec3 target){
                CamLoc = loc;
                CamTarget = target;
                name = n;
            }
            glm::vec3 CamLoc;
            glm::vec3 CamTarget;
            std::string name;
        };
        std::vector<CamLocation> CamLocations;
    };

}

#endif //FLOOF_MONSTERTRUCKSCRIPT_H
