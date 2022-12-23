#include "RaceTrackScript.h"
#include "../Components.h"
#include "../Utils.h"
#include "CheckPointScript.h"

void FLOOF::RaceTrackScript::OnCreate(FLOOF::Scene *scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);
    RaceTrack = entity;


    //make checkpoints;
    for (int i{0}; i <= 5; i++) {
        CheckPoints.emplace_back(glm::vec3(Math::RandFloat(-100.f, 100.f), -45.f, Math::RandFloat(-100.f, 100.f)));
    }
    TimePoints.resize(CheckPoints.size());
    BestTimePoints.resize(CheckPoints.size());

    //bspline generate racetrack
    auto &spline = scene->AddComponent<BSplineComponent>(RaceTrack, CheckPoints);

    //create checkpoints
    for (auto loc: CheckPoints) {
        std::string name = "Checkpoint";
        auto ent = CreateEntity(name, RaceTrack);

        auto &transform = m_Scene->GetComponent<TransformComponent>(ent);
        glm::vec3 scale{5.f};
        glm::vec3 rotation{0.f};

        transform.Scale = scale;
        transform.Position = loc;
        transform.Rotation = rotation;
        m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<CheckPointScript>(), m_Scene, ent);
        CheckPointEntities.emplace_back(ent);
    }
    auto &script = m_Scene->GetComponent<NativeScriptComponent>(CheckPointEntities[ActiveCheckPoint]);
    auto cpScript = dynamic_cast<CheckPointScript *>(script.Script.get());
    if (cpScript)
        cpScript->SetActive(true);
}

void FLOOF::RaceTrackScript::OnUpdate(float deltaTime) {
    NativeScript::OnUpdate(deltaTime);

}

void FLOOF::RaceTrackScript::NextCheckPoint() {
    //disable last hit checkpoint
    if (ActiveCheckPoint == 0) {
        TimePoints[ActiveCheckPoint] = Timer::GetTime();
    }
    {
        auto &script = m_Scene->GetComponent<NativeScriptComponent>(CheckPointEntities[ActiveCheckPoint]);
        auto cpScript = dynamic_cast<CheckPointScript *>(script.Script.get());
        if (cpScript)
            cpScript->SetActive(false);
    }

    ActiveCheckPoint++;
    if (ActiveCheckPoint >= CheckPoints.size()) {
        ActiveCheckPoint = 0;
        double LapTime = Timer::GetTimeSince(TimePoints[0]);
        if (LapTime < FastestLapTime)
            FastestLapTime = LapTime;
        TotalLapRuns++;

        //reset TimePoints
        TimePoints.clear();
        TimePoints.resize(CheckPoints.size());
    }
    TimePoints[ActiveCheckPoint] = Timer::GetTime();
    //activate next checkpoint
    {
        auto &script = m_Scene->GetComponent<NativeScriptComponent>(CheckPointEntities[ActiveCheckPoint]);
        auto cpScript = dynamic_cast<CheckPointScript *>(script.Script.get());
        if (cpScript)
            cpScript->SetActive(true);
    }


}

std::vector<std::pair<int, double>> FLOOF::RaceTrackScript::GetCheckPointTimes() {
    std::vector<std::pair<int, double>> times;

    for (int i{1}; i <= TimePoints.size(); i++) {
        double t{0};

        if (i < TimePoints.size())
            t = Timer::Delta(TimePoints[i], TimePoints[i - 1]);

        if (i - 1 == ActiveCheckPoint && i != 1)
            t = Timer::Delta(Timer::GetTime(), TimePoints[i - 1]);

        if (t > 0 && t < BestTimePoints[i] && i < BestTimePoints.size()) {
            BestTimePoints[i] = t;
        }
        if (t < 0)
            t = 0;
        times.emplace_back(i, t);
    }

    return times;
}
