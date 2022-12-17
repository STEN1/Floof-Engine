#include "RaceTrackScript.h"
#include "../Components.h"
#include "../Utils.h"
#include "CheckPointScript.h"

void FLOOF::RaceTrackScript::OnCreate(FLOOF::Scene *scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);
    RaceTrack = entity;


    //make checkpoints;
    for (int i{0}; i <= 10; i++) {
        CheckPoints.emplace_back(glm::vec3(Math::RandFloat(-100.f, 100.f), -35.f, Math::RandFloat(-100.f, 100.f)));
    }

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
    {
        auto &script = m_Scene->GetComponent<NativeScriptComponent>(CheckPointEntities[ActiveCheckPoint]);
        auto cpScript = dynamic_cast<CheckPointScript *>(script.Script.get());
        if (cpScript)
            cpScript->SetActive(false);
    }

    ActiveCheckPoint++;
    if (ActiveCheckPoint >= CheckPoints.size()) {
        ActiveCheckPoint = 0;
    }

    //activate next checkpoint
    {
        auto &script = m_Scene->GetComponent<NativeScriptComponent>(CheckPointEntities[ActiveCheckPoint]);
        auto cpScript = dynamic_cast<CheckPointScript *>(script.Script.get());
        if (cpScript)
            cpScript->SetActive(true);
    }


}
