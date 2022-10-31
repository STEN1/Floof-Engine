#include "../Floof.h"
#include <unordered_map>
#include <string>
#include "Mesh.h"
#include "btBulletDynamicsCommon.h"

namespace FLOOF {
    class ModelManager {
        ModelManager();
    public:

        ModelMesh LoadModelMesh(const std::string& path);
        void ModelMeshDestroyed(std::string& path);

        struct btModelData{
            std::vector<btVector3> btVertices;
            std::vector<int> btIndices;
            uint32_t VertCount;
        };
        btModelData LoadbtModel(const std::string& path, const glm::vec3 scale);

        void DestroyAll();

        static ModelManager& Get()
        {
            static ModelManager mm;
            return mm;
        }

    private:
        struct ModelData {
            ModelMesh Model;
            uint32_t RefCount = 0;
        };

        std::unordered_map<std::string, ModelData> m_MeshCache;
        std::unordered_map<std::string, btModelData> m_btMeshCache;
    };
}