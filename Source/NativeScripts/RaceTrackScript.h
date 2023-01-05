#ifndef FLOOF_RACETRACKSCRIPT_H
#define FLOOF_RACETRACKSCRIPT_H

#include "NativeScript.h"
#include "../Timer.h"
#include <limits>

namespace FLOOF{
    class RaceTrackScript : public NativeScript {
    public:
        void OnCreate(Scene *scene, entt::entity entity) override;

        void OnUpdate(float deltaTime) override;

        void NextCheckPoint();

        //checkpoint nmb, time in double
        std::vector<std::pair<int,double>> GetCheckPointTimes();
        double GetFastetTime(){
            return FastestLapTime;
        };
        int GetLapCount(){
            return TotalLapRuns;
        }
        std::vector<double> GetBestTimePoints(){
            return BestTimePoints;
        }
    private:
        entt::entity RaceTrack;

        double FastestLapTime{ std::numeric_limits<double>::max() };
        int TotalLapRuns{0};
        std::vector<TimePoint> TimePoints;
        std::vector<double> BestTimePoints;

        std::vector<entt::entity> CheckPointEntities;
        struct tform{
            glm::vec3 pos;
            glm::vec3 scale;
            glm::vec3 rot;
        };
        std::vector<tform> CheckTform;

        int ActiveCheckPoint{0};

    };
}



#endif //FLOOF_RACETRACKSCRIPT_H
