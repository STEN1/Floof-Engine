#pragma once

#include "Math.h"
#include "Renderer/VulkanRenderer.h"
#include "Floof.h"
#include <chrono>
#include <entt/entt.hpp>

#include "al.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBody.h"
#include "NativeScripts/NativeScript.h"

#include <pytypedefs.h>

namespace FLOOF {

    struct Relationship {
        entt::entity Parent = entt::null;
        std::vector<entt::entity> Children;
    };

    struct TagComponent {
        std::string Tag = "Entity";
    };

    struct TransformComponent {
        inline static constexpr bool in_place_delete = true;

        TransformComponent* Parent = nullptr;

        glm::vec3 Position = glm::vec3(0.f);
        glm::vec3 Rotation = glm::vec3(0.f);
        glm::vec3 Scale = glm::vec3(1.f);

        glm::mat4 GetLocalTransform() const {
            return glm::translate(Position)
                * glm::toMat4(glm::quat(Rotation))
                * glm::scale(Scale);
        }

        glm::mat4 GetTransform() const {
            TransformComponent* parent = Parent;

            glm::mat4 transform = GetLocalTransform();

            while (parent) {
                transform = parent->GetLocalTransform() * transform;
                parent = parent->Parent;
            }
            return transform;
        }
    };

    struct MeshComponent {
        MeshComponent(const std::string& objPath);
        MeshComponent(const std::vector<MeshVertex>& vertexData, const std::vector<uint32_t>& indexData);
        MeshComponent(const std::vector<ColorNormalVertex>& vertexData, const std::vector<uint32_t>& indexData);
        MeshComponent(const std::vector<MeshVertex>& vertexData);
        ~MeshComponent();

        void Draw(VkCommandBuffer commandBuffer);

        struct MeshData {
            VulkanBufferData VertexBuffer{};
            VulkanBufferData IndexBuffer{};
            uint32_t VertexCount{};
            uint32_t IndexCount{};
            std::string Path{};
        };

        MeshData Data{};

        static void ClearMeshDataCache();
    private:
        bool m_IsCachedMesh = false;
        inline static std::unordered_map<std::string, MeshData> s_MeshDataCache;
    };

    struct LineMeshComponent {
        LineMeshComponent(const std::vector<ColorVertex>& vertexData);
        ~LineMeshComponent();

        void Draw(VkCommandBuffer commandBuffer);

        void UpdateBuffer(const std::vector<ColorVertex>& vertexData);
        VulkanBufferData VertexBuffer{};
        uint32_t VertexCount{};
        uint32_t MaxVertexCount{};
    };

    struct PointCloudComponent {
        PointCloudComponent(const std::vector<ColorVertex>& vertexData);
        ~PointCloudComponent();

        void Draw(VkCommandBuffer commandBuffer);

        VulkanBufferData VertexBuffer{};
        uint32_t VertexCount{};
    };

    struct TextureComponent {
        TextureComponent(const std::string& path);
        ~TextureComponent();

        void Bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

        struct TextureData {
            VulkanCombinedTextureSampler CombinedTextureSampler{};
            VkDescriptorSet DesctriptorSet{};
            std::string Path{};
        };

        TextureData Data{};

        static void ClearTextureDataCache();
    private:
        inline static std::unordered_map<std::string, TextureData> s_TextureDataCache;
    };

    struct CameraComponent {
        CameraComponent(glm::vec3 position);
        glm::mat4 GetVP(float fov, float aspect, float near, float far);
        void MoveForward(float amount);
        void MoveRight(float amount);
        void MoveUp(float amount);
        void Pitch(float amount);
        void Yaw(float amount);
        glm::vec3 Position;
        glm::vec3 Forward;
        glm::vec3 Up;
        glm::vec3 Right;
        float FOV = 1.f;
        float Near = 0.1f;
        float Far = 100.f;
        float Aspect = 16.f / 9.f;
    };

    class BSplineComponent {
    public:
        BSplineComponent(const std::vector<glm::vec3>& controllPoints);
        BSplineComponent();
        void Update(const std::vector<glm::vec3>& controllPoints);
        void AddControllPoint(const glm::vec3& point);
        glm::vec3 EvaluateBSpline(float t);
        float TMin = 0.f;
        float TMax = 0.f;
        inline static constexpr int D = 2;
        bool empty() { return ControllPoints.empty(); }
        void clear() { ControllPoints.clear(); KnotPoints.clear(); TMin = 0.f; TMax = 0.f; }
        unsigned long size() { return ControllPoints.size(); }
        bool Isvalid();

    private:
        std::vector<int> KnotPoints;
        std::vector<glm::vec3> ControllPoints;
        int FindKnotInterval(float t);
    };
    namespace bt {
        enum CollisionPrimitive {
            ConvexHull = 0,
            Box = 1,
            Sphere,
            Capsule,
            Cylinder,
            Cone,
        };
    }
    struct RigidBodyComponent{
        RigidBodyComponent(glm::vec3 location, glm::vec3 scale, const float mass, bt::CollisionPrimitive shape);
        RigidBodyComponent(glm::vec3 location, glm::vec3 scale, const float mass,const std::string convexShape);
        std::shared_ptr<btRigidBody> RigidBody{nullptr};
        std::shared_ptr<btCollisionShape> CollisionShape{nullptr};
        btTransform Transform;
        std::shared_ptr<btDefaultMotionState> DefaultMotionState{nullptr};

        void transform(const glm::vec3 location, const glm::vec3 rotation,const glm::vec3 scale);

        const bt::CollisionPrimitive Primitive;
        void wakeup();
    private:
        void InitializeBasicPhysics(const float mass);
        const glm::vec3 DefaultScale;
    };
    struct SoftBodyComponent{
        SoftBodyComponent(const float stiffness, const float conservation,const float mass,btSoftBody* body);
        btSoftBody* SoftBody{nullptr};
        std::shared_ptr<btCollisionShape> CollisionShape{nullptr};
    };

    struct ScriptComponent{
        ScriptComponent(const std::string PyScript);
        ~ScriptComponent();
        std::string Script;
        std::string ModuleName;

        PyObject* Pname;
        PyObject* Pmodule;
        void RunScript();
        void updateScripts();

        void ReloadScript();
       void OnCreate();
       void OnUpdate(const float deltatime);
    };

	struct SoundSourceComponent {
        SoundSourceComponent();

        glm::vec3 position{ 1.0f,0.f,0.f };
        glm::vec3 velocity{ 0.0f,0.f,0.f };
        float pitch{ 1.f };
        float gain{ 1.f };
        bool looping{ false };
        ALuint m_Source;
        ALuint uint32_t;

	};

    struct PointLightComponent {
        glm::vec4 diffuse = { 0.8f, 0.4f, 0.2f, 0.f };
        glm::vec4 ambient = glm::vec4(0.4f, 0.4f, 0.4f, 0.f) * 0.1f;

        float lightRange = 50.f;

        struct PointLight {
            glm::vec4 position;
            glm::vec4 diffuse;
            glm::vec4 ambient;
            float linear;
            float quadratic;
            float lightRange;
            float pad;
        };
    };

    struct NativeScriptComponent {
        NativeScriptComponent() = delete;
        NativeScriptComponent(std::unique_ptr<NativeScript> script, std::shared_ptr<Scene> scene, entt::entity entity)
            : Script(std::move(script)) 
        {
            Script->OnCreate(scene, entity);
        }
        std::unique_ptr<NativeScript> Script;
    };
}

