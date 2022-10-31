#pragma once
#include "Physics.h"
#include <vector>
#include <string>
#include "btBulletDynamicsCommon.h"

namespace FLOOF {
    namespace Utils {
        std::vector<MeshVertex> GetVisimVertexData(const std::string& path);
        std::vector<Triangle> GetVisimTriangles(const std::string& path);
        std::vector<MeshVertex> MakeBall(int subdivisions, float radius);
        glm::vec3 CalcBarycentric(glm::vec3 position, const Triangle& triangle);
        bool IsPointInsideTriangle(const glm::vec3& position, const Triangle& triangle);
        std::vector<ColorVertex> LineVertexDataFromObj(const std::string& path);
        std::vector<ColorVertex> MakeBox(glm::vec3 extents, glm::vec3 color);

        glm::vec3 btToglm(const btVector3);
        btVector3 glmTobt(const glm::vec3);
    }
}