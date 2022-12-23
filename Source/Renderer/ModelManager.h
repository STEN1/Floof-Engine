#pragma once

#include "../Floof.h"
#include <unordered_map>
#include <string>
#include "Mesh.h"
#include "btBulletDynamicsCommon.h"
#include "../Objloader.h"

namespace FLOOF {
    struct btModelData {
        std::vector<btVector3> btVertices{};
        std::vector<int> btIndices{};
        uint32_t VertCount{};
        std::vector<btScalar> GetVertices();
    private:
        std::vector<btScalar> ScalarVertices;
    };

    class ModelManager {
        ModelManager() = delete;
    public:
        static std::vector<MeshData> LoadModelMesh(const std::string& path);
        static btModelData LoadbtModel(const std::string& path, const glm::vec3 scale);
        static VulkanBufferData GetSkyboxCube();
        static VulkanBufferData GetNDCRect();

        static void ModelMeshDestroyed(const std::string& path);
        static void DestroyAll();

    private:
        inline static std::unordered_map<std::string, std::vector<MeshData>> s_MeshCache;
        inline static std::unordered_map<std::string, btModelData> s_btMeshCache;

        inline static VulkanBufferData s_SkyboxCube{};
        inline static VulkanBufferData s_NDCRect{};
        inline static VulkanBufferData s_NDCRectFlipped{};
    };
}