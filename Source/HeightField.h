#ifndef FLOOF_HEIGHTFIELD_H
#define FLOOF_HEIGHTFIELD_H

#include "Math.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
#include "Renderer/Mesh.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"

namespace FLOOF {
    class HeightField {
    public:
        HeightField(std::vector<glm::vec3> vertices,int height, int width, float maxy, float miny);
        HeightField(std::vector<MeshVertex> meshVertex, int height, int width, float maxy, float miny);
        ~HeightField();

        btHeightfieldTerrainShape* mHeightfieldShape;
        btBvhTriangleMeshShape* TriangleMesh;

        std::vector<double> heightdata;
        std::vector<double> getHeightData(std::vector<glm::vec3> vertices);
        std::vector<glm::vec3> meshVertToGlm(std::vector<MeshVertex> meshvert);
    };

    class TriangleCollector : public btTriangleCallback{
    public:
        TriangleCollector();
        ~TriangleCollector();
        std::vector<MeshVertex> vertOut;
        std::vector<uint32_t> indicesOut;
        virtual void processTriangle(btVector3* tris, int partId, int triangleIndex) override;

        int height,width;

    };
}

#endif //FLOOF_HEIGHTFIELD_H
