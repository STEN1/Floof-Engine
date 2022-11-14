#include "MonsterTruckScript.h"
#include "../Components.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/ModelManager.h"

void FLOOF::MonsterTruckScript::OnCreate(std::shared_ptr<Scene> scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);

    {
        frame = entity;
        auto& mesh = scene->AddComponent<StaticMeshComponent>(frame, "Assets/MonsterTruck/MonstertruckFrame.fbx");
        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_albedo.jpg");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_metallic.jpg");
        mesh.meshes[0].MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_AO.jpg");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_normal.jpg");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_roughness.jpg");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        auto& transform = scene->GetComponent<TransformComponent>(frame);
        transform.Scale = glm::vec3(5.f);
        transform.Rotation = glm::vec3(1.5f, 0.f, 0.f);
    }
    {
        Wheel_fr = scene->CreateEntity("Wheel Front left", frame);
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fr, "Assets/MonsterTruck/LPWheelFixed.fbx");
        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
        mesh.meshes[0].MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        auto& transform = scene->GetComponent<TransformComponent>(Wheel_fr);
        transform.Position = glm::vec3(-7.f, -4.f, 2.f);
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f, 0.f, -1.5f);

        auto& frameTrans = scene->GetComponent<TransformComponent>(frame);
        scene->AddComponent<RigidBodyComponent>(Wheel_fr, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
    }
    {
        Wheel_fl = scene->CreateEntity("Wheel Front right", frame);
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fl, "Assets/MonsterTruck/LPWheelFixed.fbx");
        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
        mesh.meshes[0].MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        auto& transform = scene->GetComponent<TransformComponent>(Wheel_fl);
        transform.Position = glm::vec3(-7.f, 4.f, 2.f);
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f, 0.f, 1.5f);
    }
    {
        Wheel_br = scene->CreateEntity("Wheel Back right", frame);
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_br, "Assets/MonsterTruck/LPWheelFixed.fbx");
        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
        mesh.meshes[0].MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        auto& transform = scene->GetComponent<TransformComponent>(Wheel_br);
        transform.Position = glm::vec3(7.f, 4.f, 2.f);
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f, 0.f, 1.5f);
    }
    {
        Wheel_bl = scene->CreateEntity("Wheel back left", frame);
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_bl, "Assets/MonsterTruck/LPWheelFixed.fbx");
        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
        mesh.meshes[0].MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        auto& transform =scene->GetComponent<TransformComponent>(Wheel_bl);
        transform.Position = glm::vec3(7.f,-4.f,2.f);
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f,0.f,-1.5f);
    }


}

void FLOOF::MonsterTruckScript::OnUpdate(float deltaTime) {
    NativeScript::OnUpdate(deltaTime);


}
