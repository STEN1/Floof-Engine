#pragma once
#include <vector>
#include <string>
#include "btBulletDynamicsCommon.h"
#include "Math.h"
#include "Renderer/Vertex.h"

namespace FLOOF {
    namespace Utils {
        std::vector<MeshVertex> GetVisimVertexData(const std::string& path);
        std::vector<MeshVertex> MakeBall(int subdivisions, float radius);
        std::vector<ColorVertex> LineVertexDataFromObj(const std::string& path);
        std::vector<ColorVertex> MakeBox(glm::vec3 extents, glm::vec3 color);

        glm::vec3 btToglm(const btVector3);
        btVector3 glmTobt(const glm::vec3);
    }
}