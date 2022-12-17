#pragma once

#include "Math.h"
#include "Renderer/VulkanRenderer.h"
#include "Floof.h"
#include <chrono>
#include <entt/entt.hpp>
#include <initializer_list>
#include <cstdarg>
#include "al.h"
#include "Renderer/Mesh.h"
#include "Renderer/LandscapeMesh.h"
#include "Renderer/Texture.h"
#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBody.h"
#include "NativeScripts/NativeScript.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "PointLightComponent.h"
#include "HeightmapLoader.h"
//#include "SoundComponent.h"
// tmp remove python #include <pytypedefs.h>
#include "HeightField.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include <unordered_map>

namespace FLOOF {

    struct Relationship {
        entt::entity Parent = entt::null;
        std::vector<entt::entity> Children;
    };

    struct TagComponent {
        std::string Tag = "Entity";
    };

    struct PointCloudComponent {
        PointCloudComponent(const std::vector<ColorVertex> &vertexData);

        ~PointCloudComponent();

        void Draw(VkCommandBuffer commandBuffer);

        VulkanBufferData VertexBuffer{};
        uint32_t VertexCount{};
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
        RigidBodyComponent(glm::vec3 location, glm::vec3 scale,glm::vec3 rotation, const float mass, bt::CollisionPrimitive shape);

        RigidBodyComponent(glm::vec3 location, glm::vec3 scale,glm::vec3 rotation, const float mass, const std::string convexShape);

        std::shared_ptr<btRigidBody> RigidBody{nullptr};
        std::shared_ptr<btCollisionShape> CollisionShape{nullptr};
        btTransform Transform;
        std::shared_ptr<btDefaultMotionState> DefaultMotionState{nullptr};
        std::shared_ptr<btGhostObject> GhostObject;

        void transform(const glm::vec3 location, const glm::vec3 rotation, const glm::vec3 scale);

        const bt::CollisionPrimitive Primitive;

        void wakeup();

        //Set CollisionDispatcher
        void setCollisionDispatcher(void* ptr);

    private:
        void InitializeBasicPhysics(const float mass);

        const glm::vec3 DefaultScale;
    };
    struct TriggerVolumeComponent{
        TriggerVolumeComponent(glm::vec3 location, glm::vec3 scale,glm::vec3 rotation, const float mass, bt::CollisionPrimitive shape);
        std::shared_ptr<btRigidBody> RigidBody{nullptr};
        std::shared_ptr<btCollisionShape> CollisionShape{nullptr};
        btTransform Transform;
        std::shared_ptr<btDefaultMotionState> DefaultMotionState{nullptr};

        void setCollisionDispatcher(void* ptr);
        void transform(const glm::vec3 location, const glm::vec3 rotation, const glm::vec3 scale);
    };

    struct SoftBodyComponent {
        SoftBodyComponent(const float stiffness, const float conservation, const float mass, btSoftBody *body);

        ~SoftBodyComponent();

        btSoftBody *SoftBody{nullptr}; // i dont have owernship
        std::shared_ptr<btCollisionShape> CollisionShape{nullptr};
    };

    /* Temp removed Python scripting
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
*/


    struct NativeScriptComponent {
        NativeScriptComponent() = delete;

        NativeScriptComponent(std::unique_ptr<NativeScript> script, Scene* scene, entt::entity entity)
                : Script(std::move(script)) {
            Script->OnCreate(scene, entity);
        }

        std::unique_ptr<NativeScript> Script;
    };

    struct LandscapeComponent {
        LandscapeComponent(const char* map, const char* texture);
        ~LandscapeComponent();
        
        LandscapeMesh meshData;

        Material TestMaterial;

        HeightmapLoader* landscape;

        TriangleCollector triangleCol;
        HeightField* HeightFieldShape;
    };
    struct PlayerControllerComponent{
        PlayerControllerComponent(int player):mPlayer(player){};
        ~PlayerControllerComponent(){};

        int mPlayer;
    };
}

