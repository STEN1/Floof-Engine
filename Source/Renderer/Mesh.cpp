#include "Mesh.h"
#include "ModelManager.h"

namespace FLOOF {
    StaticMeshComponent::StaticMeshComponent(const std::string& path) {
        meshes = ModelManager::LoadModelMesh(path);
        uint32_t lastSlashIndex{};
        for (uint32_t i = 0; i < path.size(); i++) {
            if (path[i] == '/')
                lastSlashIndex = i;
        }
        std::string assetPath = path.substr(0, lastSlashIndex + 1);
        for (auto& mesh : meshes) {
            if (!mesh.MeshMaterial.Diffuse.Path.empty()) {
                mesh.MeshMaterial.Diffuse = Texture(assetPath + mesh.MeshMaterial.Diffuse.Path);
            }
            if (!mesh.MeshMaterial.Normals.Path.empty()) {
                mesh.MeshMaterial.Normals = Texture(assetPath + mesh.MeshMaterial.Normals.Path);
            }
            if (!mesh.MeshMaterial.Metallic.Path.empty()) {
                mesh.MeshMaterial.Metallic = Texture(assetPath + mesh.MeshMaterial.Metallic.Path);
            }
            if (!mesh.MeshMaterial.Roughness.Path.empty()) {
                mesh.MeshMaterial.Roughness = Texture(assetPath + mesh.MeshMaterial.Roughness.Path);
            }
            if (!mesh.MeshMaterial.AO.Path.empty()) {
                mesh.MeshMaterial.AO = Texture(assetPath + mesh.MeshMaterial.AO.Path);
            }
            mesh.MeshMaterial.UpdateDescriptorSet();
        }
    }
}