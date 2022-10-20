#include "../Floof.h"
#include <unordered_map>
#include <string>
#include "Mesh.h"

namespace FLOOF {
    class ModelManager {
    public:
        ModelManager();
    private:
        struct ModelData {
            std::vector<ModelMesh> Meshes;
            uint32_t RefCount = 0;
        };
        std::unordered_map<std::string, ModelData> s_MeshCache;
    };
}