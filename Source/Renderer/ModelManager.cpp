#include "ModelManager.h"
#include "../Objloader.h"
#include "VulkanRenderer.h"
#include "LandscapeMesh.h"

namespace FLOOF {
    std::vector<MeshData> ModelManager::LoadModelMesh(const std::string& path) {

        auto it = s_MeshCache.find(path);
        if (it != s_MeshCache.end()) {
            return it->second;
        }

        std::vector<MeshData> ret;
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
            meshData.MeshName = mesh.name;
            meshData.MeshMaterial.Diffuse.Path = mesh.material.DiffusePath;
            meshData.MeshMaterial.Normals.Path = mesh.material.NormalsPath;
            meshData.MeshMaterial.Metallic.Path = mesh.material.MetallicPath;
            meshData.MeshMaterial.Roughness.Path = mesh.material.RoughnessPath;
            meshData.MeshMaterial.AO.Path = mesh.material.AOPath;
            meshData.MeshMaterial.Opacity.Path = mesh.material.OpacityPath;
            meshData.MeshMaterial.Name = mesh.material.Name;
            ret.emplace_back(meshData);
        }

        s_MeshCache[path] = ret;

        return ret;
    }

    void ModelManager::ModelMeshDestroyed(const std::string& path) {
    }

    void ModelManager::DestroyAll() {
        auto* renderer = VulkanRenderer::Get();
        for (auto& [path, data] : s_MeshCache) {
            // TODO: Models also have material data. Need to handle that here?

            for (auto& mesh : data) {
                vmaDestroyBuffer(renderer->m_Allocator, mesh.IndexBuffer.Buffer, mesh.IndexBuffer.Allocation);
                vmaDestroyBuffer(renderer->m_Allocator, mesh.VertexBuffer.Buffer, mesh.VertexBuffer.Allocation);
            }
        }
        if (s_SkyboxCube.Buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(renderer->GetAllocator(), s_SkyboxCube.Buffer, s_SkyboxCube.Allocation);
        }
        if (s_NDCRect.Buffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(renderer->GetAllocator(), s_NDCRect.Buffer, s_NDCRect.Allocation);
        }
    }

    btModelData ModelManager::LoadbtModel(const std::string &path,const glm::vec3 scale) {

        auto pathwithScale = path + std::to_string(scale.x) + std::to_string(scale.y)+ std::to_string(scale.z) ;
        auto it = s_btMeshCache.find(pathwithScale);
        if (it != s_btMeshCache.end()) {
            std::cout << "Getting btModel data from cache" << std::endl;
            return it->second;
        }

        AssimpLoader loader(path);
        btModelData meshData;
        // Loader returns staticMesh, eventually it will return collision mesh too
        const auto& assimpStaticMesh = loader.GetAssimpStaticMesh();
        ASSERT(assimpStaticMesh.meshes.size() > 0);
        for (const auto& mesh : assimpStaticMesh.meshes)
        {

            for(auto& vert : mesh.vertices){
                auto pos = vert.Pos * scale;
                meshData.btVertices.emplace_back(btVector3(pos.x,pos.y,pos.z));
            }
            for(auto& ind : mesh.indices){
                meshData.btIndices.emplace_back(ind);
            }
            meshData.VertCount += mesh.vertices.size();
        }

        s_btMeshCache[pathwithScale] = meshData;
        return meshData;
    }
    static const std::vector<SimpleVertex> cubemapVertices = {
        // positions          
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},
        SimpleVertex{{-1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f,  1.0f, -1.0f}},
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},

        SimpleVertex{{-1.0f, -1.0f,  1.0f}},
        SimpleVertex{{-1.0f, -1.0f, -1.0f}},
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},
        SimpleVertex{{-1.0f,  1.0f,  1.0f}},
        SimpleVertex{{-1.0f, -1.0f,  1.0f}},

        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},

        SimpleVertex{{-1.0f, -1.0f,  1.0f}},
        SimpleVertex{{-1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f, -1.0f,  1.0f}},
        SimpleVertex{{-1.0f, -1.0f,  1.0f}},

        SimpleVertex{{-1.0f,  1.0f, -1.0f}},
        SimpleVertex{{1.0f,  1.0f, -1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{-1.0f,  1.0f,  1.0f}},
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},

        SimpleVertex{{-1.0f, -1.0f, -1.0f}},
        SimpleVertex{{-1.0f, -1.0f,  1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{-1.0f, -1.0f,  1.0f}},
        SimpleVertex{{1.0f, -1.0f,  1.0}}
    };
    VulkanBufferData ModelManager::GetSkyboxCube()
    {
        if (s_SkyboxCube.Buffer == VK_NULL_HANDLE) {
            auto* renderer = VulkanRenderer::Get();
            s_SkyboxCube = renderer->CreateVertexBuffer(cubemapVertices);
        }
        return s_SkyboxCube;
    }

    static const std::vector<MeshVertex> ndcRectVertexData = {
        MeshVertex{glm::vec3{-1.f, 1.f, 0.f}, glm::vec3{0.f, 0.f, 1.f}, glm::vec2{0.f, 0.f,}},
        MeshVertex{glm::vec3{-1.f, -1.f, 0.f}, glm::vec3{0.f, 0.f, 1.f}, glm::vec2{0.f, 1.f}},
        MeshVertex{glm::vec3{1.f, 1.f, 0.f}, glm::vec3{0.f, 0.f, 1.f}, glm::vec2{1.f, 0.f}},
        MeshVertex{glm::vec3{1.f, -1.f, 0.f}, glm::vec3{0.f, 0.f, 1.f}, glm::vec2{1.f, 1.f}},
    };

    VulkanBufferData ModelManager::GetNDCRect()
    {
        if (s_NDCRect.Buffer == VK_NULL_HANDLE) {
            auto* renderer = VulkanRenderer::Get();
            s_NDCRect = renderer->CreateVertexBuffer(ndcRectVertexData);
        }
        return s_NDCRect;
    }


    std::vector<btScalar> btModelData::GetVertices() {

        ScalarVertices.resize(VertCount*3);
        std::cout << "mesh vertices"<< std::endl;
        for(int i{0} ,j{0}; i < VertCount; j+=3, i++){
            ScalarVertices[j] = btVertices[i].x();
            ScalarVertices[j+1] = btVertices[i].y();
            ScalarVertices[j+2] = btVertices[i].z();

        }
        return ScalarVertices;
    }
}