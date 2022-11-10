#include "MonsterTruckScript.h"
#include "../Components.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/ModelManager.h"

void FLOOF::MonsterTruckScript::OnCreate(std::shared_ptr<Scene> scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);

    {
        frame = entity;
        auto& mesh = scene->AddComponent<StaticMeshComponent>(frame);
        mesh.meshes = ModelManager::Get().LoadModelMesh("Assets/MonsterTruck/MonstertruckFrame.fbx");
        auto& texture = scene->AddComponent<TextureComponent>(frame, "Assets/MonsterTruck/texturesFrame/HarmaaRunko_albedo.jpg");

        auto& transform =scene->GetComponent<TransformComponent>(frame);
        transform.Scale = glm::vec3(5.f);
        transform.Rotation = glm::vec3(1.5f,0.f,0.f);
    }
    {
        Wheel_fr = scene->CreateEntity("Wheel Front left", frame);
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fr);
        mesh.meshes = ModelManager::Get().LoadModelMesh("Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& texture = scene->AddComponent<TextureComponent>(Wheel_fr, "Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");

        auto& transform =scene->GetComponent<TransformComponent>(Wheel_fr);
        transform.Position = glm::vec3(-7.f,-4.f,2.f);
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f,0.f,-1.5f);
    }
    {
        Wheel_fl = scene->CreateEntity("Wheel Front right", frame);
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fl);
        mesh.meshes = ModelManager::Get().LoadModelMesh("Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& texture = scene->AddComponent<TextureComponent>(Wheel_fl, "Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");

        auto& transform =scene->GetComponent<TransformComponent>(Wheel_fl);
        transform.Position = glm::vec3(-7.f,4.f,2.f);
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f,0.f,1.5f);
    }
    {
        Wheel_br = scene->CreateEntity("Wheel Back right", frame);
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_br);
        mesh.meshes = ModelManager::Get().LoadModelMesh("Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& texture = scene->AddComponent<TextureComponent>(Wheel_br, "Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");

        auto& transform =scene->GetComponent<TransformComponent>(Wheel_br);
        transform.Position = glm::vec3(7.f,4.f,2.f);
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f,0.f,1.5f);
    }
    {
        Wheel_bl = scene->CreateEntity("Wheel back left",frame);
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_bl);
        mesh.meshes = ModelManager::Get().LoadModelMesh("Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& texture = scene->AddComponent<TextureComponent>(Wheel_bl, "Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");

        auto& transform =scene->GetComponent<TransformComponent>(Wheel_bl);
        transform.Position = glm::vec3(7.f,-4.f,2.f);
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f,0.f,-1.5f);
    }


}

void FLOOF::MonsterTruckScript::OnUpdate(float deltaTime) {
    NativeScript::OnUpdate(deltaTime);


}
