#include "ModelManager.h"
#include "../objloader.h"
#include "VulkanRenderer.h"

namespace FLOOF {
    ModelManager::ModelManager() {
    }

    ModelMesh ModelManager::LoadModelMesh(const std::string& path) {

        auto it = m_MeshCache.find(path);
        if (it != m_MeshCache.end()) {
            it->second.RefCount++;
            return it->second.Model;
        }

        ModelData modelData;
        AssimpLoader loader(path);
        auto* renderer = VulkanRenderer::Get();

        // Loader returns staticMesh, eventually it will return collision mesh too
        const auto& assimpStaticMesh = loader.GetAssimpStaticMesh();
        
        for (const auto& mesh : assimpStaticMesh.meshes)
        {
            MeshData meshData;
            meshData.VertexBuffer = renderer->CreateVertexBuffer(mesh.vertices);
            meshData.IndexBuffer = renderer->CreateIndexBuffer(mesh.indices);
            meshData.VertexCount = mesh.vertices.size();
            meshData.IndexCount = mesh.indices.size();
            modelData.Model.meshes.emplace_back(meshData);
        }
        modelData.Model.Path = path;

        auto& data = m_MeshCache[path] = modelData;
        data.RefCount++;

        return modelData.Model;
    }

    void ModelManager::ModelMeshDestroyed(std::string& path) {
    }

    void ModelManager::DestroyAll() {
        auto* renderer = VulkanRenderer::Get();
        for (auto& [path, data] : m_MeshCache) {
            // TODO: Models also have material data. Need to handle that here?

            for (auto& mesh : data.Model.meshes) {
                vmaDestroyBuffer(renderer->m_Allocator, mesh.IndexBuffer.Buffer, mesh.IndexBuffer.Allocation);
                vmaDestroyBuffer(renderer->m_Allocator, mesh.VertexBuffer.Buffer, mesh.VertexBuffer.Allocation);
            }
        }
    }
}