#ifndef FLOOF_RACETRACKSCRIPT_H
#define FLOOF_RACETRACKSCRIPT_H

#include "NativeScript.h"

namespace FLOOF{
    class RaceTrackScript : public NativeScript {
    public:
        void OnCreate(Scene *scene, entt::entity entity) override;

        void OnUpdate(float deltaTime) override;

        void NextCheckPoint();
    private:
        entt::entity RaceTrack;

        std::vector<glm::vec3> CheckPoints;
        std::vector<entt::entity> CheckPointEntities;

        int ActiveCheckPoint{0};

    };
}



#endif //FLOOF_RACETRACKSCRIPT_H
