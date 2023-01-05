#include "RaceTrackScript.h"
#include "../Components.h"
#include "../Utils.h"
#include "CheckPointScript.h"

void FLOOF::RaceTrackScript::OnCreate(FLOOF::Scene *scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);
    RaceTrack = entity;

    auto view = m_Scene->GetRegistry().view<PlayerControllerComponent>();
    for (auto [ent, player]: view.each()) {
        if (player.mPlayer == m_Scene->ActivePlayer) {
            player.spawnLoc = glm::vec3(-350.5f, -55.3f, -302.9f);
            player.spawnRot = glm::vec3(0.f);
        }
    }

    // Add checkpoint sound
    auto &sound = scene->AddComponent<SoundComponent>(RaceTrack, "checkpoint.wav");
    sound.GetClip("checkpoint.wav")->Volume(0.8f);

    //make checkpoints;
    tform tf;
    tf.pos = glm::vec3(-330.5f, -57.3f, -302.9f);
    tf.rot = glm::vec3(0.0f, 2.27f, 0.0f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(-390.3f, -45.2f, -91.9f);
    tf.rot = glm::vec3(0.0f, 3.14f, 0.0f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(-360.3f, 12.4f, 163.517f);
    tf.rot = glm::vec3(0.0f, 3.13f, 0.0f);
    tf.scale = glm::vec3(0.3f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(-104.8f, -47.4f, 379.4f);
    tf.rot = glm::vec3(0.0f, 4.5f, 0.0f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(0.0f, -44.f, 250.0f);
    tf.rot = glm::vec3(0.0f, 6.f, 0.0f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(243.8f, -39.4f, 130.7f);
    tf.rot = glm::vec3(0.0f, 5.5f, 0.0f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(341.9f, 13.4f, -210.f);
    tf.rot = glm::vec3(0.0f, 0.5f, 0.0f);
    tf.scale = glm::vec3(0.3f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(251.5f, -27.2f, -390.9f);
    tf.rot = glm::vec3(0.0f, 1.f, 0.0f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(84.9f, -28.6f, -283.9f);
    tf.rot = glm::vec3(0.0f, 2.5f, 0.0f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(-39.f, -29.3f, -113.5f);
    tf.rot = glm::vec3(0.2f, 2.2f, 0.25f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    tf.pos = glm::vec3(-168.5f, -55.1f, -247.f);
    tf.rot = glm::vec3(0.f, 0.f, 0.f);
    tf.scale = glm::vec3(0.2f);
    CheckTform.emplace_back(tf);

    TimePoints.resize(CheckTform.size());
    BestTimePoints.resize(CheckTform.size());

    //create checkpoints
    for (auto loc: CheckTform) {
        std::string name = "Checkpoint";
        auto ent = CreateEntity(name, RaceTrack);

        auto &transform = m_Scene->GetComponent<TransformComponent>(ent);
        transform.Scale = loc.scale;
        transform.Position = loc.pos;
        transform.Rotation = loc.rot;
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
        auto tform = GetComponent<TransformComponent>(CheckPointEntities[ActiveCheckPoint]);
        auto view = m_Scene->GetRegistry().view<PlayerControllerComponent>();
        for (auto [ent, player]: view.each()) {
            if (player.mPlayer == m_Scene->ActivePlayer) {
                player.spawnLoc = tform.Position;
                player.spawnRot = tform.Rotation;
            }
        }

    }

    ActiveCheckPoint++;
    if (ActiveCheckPoint >= CheckTform.size()) {
        ActiveCheckPoint = 0;
        double LapTime = Timer::GetTimeSince(TimePoints[0]);
        if (LapTime < FastestLapTime)
            FastestLapTime = LapTime;
        TotalLapRuns++;

        //reset TimePoints
        TimePoints.clear();
        TimePoints.resize(CheckTform.size());
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


