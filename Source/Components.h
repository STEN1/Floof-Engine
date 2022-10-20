#pragma once
#include "Math.h"

#include "Renderer/VulkanRenderer.h"
#include "Floof.h"
#include "Physics.h"
#include <chrono>
#include <entt/entt.hpp>

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
            VulkanBuffer VertexBuffer{};
            VulkanBuffer IndexBuffer{};
            uint32_t VertexCount{};
            uint32_t IndexCount{};
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
        VulkanBuffer VertexBuffer{};
        uint32_t VertexCount{};
        uint32_t MaxVertexCount{};
    };

    struct PointCloudComponent {
        PointCloudComponent(const std::vector<ColorVertex>& vertexData);
        ~PointCloudComponent();

        void Draw(VkCommandBuffer commandBuffer);

        VulkanBuffer VertexBuffer{};
        uint32_t VertexCount{};
    };

    struct TextureComponent {
        TextureComponent(const std::string& path);
        ~TextureComponent();

        void Bind(VkCommandBuffer commandBuffer);

        struct TextureData {
            VulkanCombinedTextureSampler CombinedTextureSampler{};
            VkDescriptorSet DesctriptorSet{};
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

    struct TerrainComponent {
        TerrainComponent(std::vector<std::vector<std::pair<Triangle, Triangle>>>& vertexData);
        void PrintTriangleData();
        std::vector<Triangle*> GetOverlappingTriangles(CollisionShape* shape);
        std::vector<std::vector<std::pair<Triangle, Triangle>>> Rectangles;
        std::vector<Triangle> Triangles;
        int Width;
        int Height;
        float MinY;
    };

    struct BallComponent {
        Sphere CollisionSphere;
        float Radius; // TODO dobbel lagring av radius !! fix??
        float Mass;
        float Elasticity{ 0.5f };
    };

    struct VelocityComponent {
        glm::vec3 Velocity;
        glm::vec3 Force;

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

    struct DebugComponent {};

    struct TimeComponent {
        std::chrono::high_resolution_clock::time_point CreationTime;
        std::chrono::high_resolution_clock::time_point LastPoint;
    };
}

