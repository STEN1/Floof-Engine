#include "HeightField.h"


FLOOF::HeightField::HeightField(std::vector<glm::vec3> vertices, int height, int width, float maxy, float miny) {

    auto vertheight = getHeightData(vertices);
    mHeightfieldShape = new btHeightfieldTerrainShape(height+1, width+1, &vertheight[0], miny, maxy, 1, false);
    mHeightfieldShape->buildAccelerator(16);
    mHeightfieldShape->setLocalScaling(btVector3(1.0, 1.0, 1.0));

}

FLOOF::HeightField::~HeightField() {
    delete mHeightfieldShape;
}

std::vector<double> FLOOF::HeightField::getHeightData(std::vector<glm::vec3> vertices) {

    std::vector<double> data(vertices.size());

    for (int i{0}; i < vertices.size(); i++) {
        data[i] = vertices[i].y;
    }

    return data;
}

FLOOF::HeightField::HeightField(std::vector<MeshVertex> meshVertex, int height, int width, float maxy, float miny)
    : HeightField(meshVertToGlm(meshVertex),height,width,maxy,miny) {
}

std::vector<glm::vec3> FLOOF::HeightField::meshVertToGlm(std::vector<MeshVertex> meshvert) {

    std::vector<glm::vec3> output(meshvert.size());

    for (int i{0}; i < meshvert.size(); i++) {
        output[i] = meshvert[i].Pos;
    }

    return output;
}

FLOOF::TriangleCollector::TriangleCollector() {


}

FLOOF::TriangleCollector::~TriangleCollector() {


}

void FLOOF::TriangleCollector::processTriangle(btVector3 *tris, int partId, int triangleIndex) {
    for (int k = 0; k < 3; k++) {
        MeshVertex v;
        v.Pos = glm::vec3(0.f);
        v.UV = glm::vec2(0.5f);



        btVector3 normal = (tris[0] - tris[1]).cross(tris[0] - tris[2]);
        normal.safeNormalize();
        v.Pos.x = tris[k][0];
        v.Pos.y = tris[k][1];
        v.Pos.z = tris[k][2];
        v.Normal.x = normal[0];
        v.Normal.y = normal[1];
        v.Normal.z = normal[2];
        v.UV.x = (1.f / 1025)/ v.Pos.y;
        v.UV.y = (1.f / 1025)/ v.Pos.x;

        indicesOut.emplace_back(vertOut.size());
        vertOut.emplace_back(v);
    }
}
