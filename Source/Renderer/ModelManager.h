#include "../Floof.h"
#include <unordered_map>
#include <string>
#include "Mesh.h"

namespace FLOOF {
    class ModelManager {
    public:
        ModelManager();
        ModelMesh LoadModelMesh(std::string& path);
        void ModelMeshDestroyed(std::string& path);
    private:
        struct ModelData {
            ModelMesh Model;
            uint32_t RefCount = 0;
        };
        std::unordered_map<std::string, ModelData> s_MeshCache;
    };
}