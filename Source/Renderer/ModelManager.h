#include "../Floof.h"
#include <unordered_map>
#include <string>
#include "Mesh.h"

namespace FLOOF {
    class ModelManager {
    public:
        ModelManager();
        ModelMesh LoadModelMesh(const std::string& path);
        void ModelMeshDestroyed(std::string& path);

        static ModelManager& Get()
        {
            static ModelManager mm; return mm;
        }

    private:
        struct ModelData {
            ModelMesh Model;
            uint32_t RefCount = 0;
        };
        std::unordered_map<std::string, ModelData> m_MeshCache;
    };
}