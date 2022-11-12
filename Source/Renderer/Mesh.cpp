#include "Mesh.h"
#include "ModelManager.h"

namespace FLOOF {
    StaticMeshComponent::StaticMeshComponent(const std::string& path) {
        meshes = ModelManager::LoadModelMesh(path);
    }
}