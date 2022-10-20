#include "ModelManager.h"

namespace FLOOF {
    ModelManager::ModelManager() {
    }

    ModelMesh ModelManager::LoadModelMesh(std::string& path) {
        return ModelMesh();
    }

    void ModelManager::ModelMeshDestroyed(std::string& path) {
    }
}