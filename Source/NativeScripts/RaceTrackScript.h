#ifndef FLOOF_RACETRACKSCRIPT_H
#define FLOOF_RACETRACKSCRIPT_H

#include "NativeScript.h"
#include "../Timer.h"

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

        double FastestLapTime{MAXFLOAT};
        int TotalLapRuns{0};
        std::vector<TimePoint> TimePoints;
        std::vector<double> BestTimePoints;

        std::vector<glm::vec3> CheckPoints;
        std::vector<entt::entity> CheckPointEntities;

        int ActiveCheckPoint{0};

    };
}



#endif //FLOOF_RACETRACKSCRIPT_H
