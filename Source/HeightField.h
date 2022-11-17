#ifndef FLOOF_HEIGHTFIELD_H
#define FLOOF_HEIGHTFIELD_H

#include "Math.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "Renderer/Mesh.h"

namespace FLOOF {
    class HeightField {
    public:
        HeightField(std::vector<glm::vec3> vertices);
        HeightField(std::vector<MeshVertex> meshVertex);
        ~HeightField();

        btHeightfieldTerrainShape* mHeightfieldShape;

        std::vector<double> getHeightData(std::vector<glm::vec3> vertices);
        std::vector<glm::vec3> meshVertToGlm(std::vector<MeshVertex> meshvert);
    };
}

#endif //FLOOF_HEIGHTFIELD_H
