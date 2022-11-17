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
#include "TransformComponent.h"

#include <pytypedefs.h>

class SoundManager;

namespace FLOOF {

    struct Relationship {
        entt::entity Parent = entt::null;
        std::vector<entt::entity> Children;
    };

    struct TagComponent {
        std::string Tag = "Entity";
    };

    struct LineMeshComponent {
        LineMeshComponent(const std::vector<ColorVertex> &vertexData);

        ~LineMeshComponent();

        void Draw(VkCommandBuffer commandBuffer);

        void UpdateBuffer(const std::vector<ColorVertex> &vertexData);

        VulkanBufferData VertexBuffer{};
        uint32_t VertexCount{};
        uint32_t MaxVertexCount{};
    };

    struct PointCloudComponent {
        PointCloudComponent(const std::vector<ColorVertex> &vertexData);

        ~PointCloudComponent();

        void Draw(VkCommandBuffer commandBuffer);

        VulkanBufferData VertexBuffer{};
        uint32_t VertexCount{};
    };

    struct CameraComponent {
        CameraComponent(glm::vec3 position);

        glm::mat4 GetVP(float fov, float aspect, float near, float far);
        glm::mat4 GetView();
        glm::mat4 GetPerspective(float fov, float aspect, float near, float far);

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
        BSplineComponent(const std::vector<glm::vec3> &controllPoints);

        BSplineComponent();

        void Update(const std::vector<glm::vec3> &controllPoints);

        void AddControllPoint(const glm::vec3 &point);

        glm::vec3 EvaluateBSpline(float t);

        float TMin = 0.f;
        float TMax = 0.f;
        inline static constexpr int D = 2;

        bool empty() { return ControllPoints.empty(); }

        void clear() {
            ControllPoints.clear();
            KnotPoints.clear();
            TMin = 0.f;
            TMax = 0.f;
        }

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
    struct RigidBodyComponent {
        RigidBodyComponent(glm::vec3 location, glm::vec3 scale, const float mass, bt::CollisionPrimitive shape);

        RigidBodyComponent(glm::vec3 location, glm::vec3 scale,glm::vec3 rotation, const float mass, const std::string convexShape);

        std::shared_ptr<btRigidBody> RigidBody{nullptr};
        std::shared_ptr<btCollisionShape> CollisionShape{nullptr};
        btTransform Transform;
        std::shared_ptr<btDefaultMotionState> DefaultMotionState{nullptr};

        void transform(const glm::vec3 location, const glm::vec3 rotation, const glm::vec3 scale);

        const bt::CollisionPrimitive Primitive;

        void wakeup();

    private:
        void InitializeBasicPhysics(const float mass);

        const glm::vec3 DefaultScale;
    };

    struct SoftBodyComponent {
        SoftBodyComponent(const float stiffness, const float conservation, const float mass, btSoftBody *body);

        ~SoftBodyComponent();

        btSoftBody *SoftBody{nullptr}; // i dont have owernship
        std::shared_ptr<btCollisionShape> CollisionShape{nullptr};
    };

    struct ScriptComponent {
        ScriptComponent(const std::string PyScript);

        ~ScriptComponent();

        std::string Script;
        std::string ModuleName;

        PyObject *Pname;
        PyObject *Pmodule;

        void RunScript();

        void ReloadScript();

        void OnCreate();

        void OnUpdate(const float deltatime);
    };

    struct SoundSourceComponent {
        friend class SoundManager;

    public:
        SoundSourceComponent(const std::string &path);

        ~SoundSourceComponent();

        void Volume(float volume);

        void Pitch();

        void UpdateStatus();

        void Play();

        void Stop();

        void Looping(bool looping);

        void Update();

        ALuint m_Source;
        ALuint m_Sound;
        std::string m_Path;
        float m_Volume{1.f};
        float m_Pitch{1.f};
        bool isPlaying{false};
        bool isLooping{false};


    };

    struct PointLightComponent {
        glm::vec4 diffuse = {1.0f, 0.7f, 0.5f, 0.f};
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
                : Script(std::move(script)) {
            Script->OnCreate(scene, entity);
        }

        std::unique_ptr<NativeScript> Script;
    };

    struct EngineComponent {
        EngineComponent(){};

        float EngineForce = 0.f;
        float TurnForce = 0.f;

        const float defaultBreakingForce = 10.f;
        float BreakingForce = 100.f;

        float maxEngineForce = 10000.f;  //this should be engine/velocity dependent
        float maxBreakingForce = 100.f;
        float maxVelocity = 20.f;

        float maxTurnForce = 2000.f;

        float VehicleSteering = 0.f;
        float steeringIncrement = 0.04f;
        float steeringClamp = 0.3f;

        float suspensionStiffness = 8000.f;
        float suspensionDamping = 8000.f;
        float suspensionCompression = 4.4f;

        float rollInfluence = 0.1f;  //1.0f;
        float wheelFriction = 1000;

        btScalar suspensionRestLength = 0.6;

        std::vector<btHinge2Constraint*> axles;
    };

}

