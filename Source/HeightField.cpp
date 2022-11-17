#include "HeightField.h"


FLOOF::HeightField::HeightField(std::vector<glm::vec3> vertices) {

    auto height = getHeightData(vertices);
    mHeightfieldShape = new btHeightfieldTerrainShape(17,17,&height[0],0,10.f,2,false);
    mHeightfieldShape->buildAccelerator(16);
    mHeightfieldShape->setLocalScaling(btVector3(1.0,0.5,1.0));

}

FLOOF::HeightField::~HeightField() {
    delete mHeightfieldShape;
}

std::vector<double> FLOOF::HeightField::getHeightData(std::vector<glm::vec3> vertices) {

    std::vector<double> data(vertices.size());

    for(int i {0}; i < vertices.size(); i++){
        data[i] = vertices[i].y;
    }

    return data;
}

FLOOF::HeightField::HeightField(std::vector<MeshVertex> meshVertex) : HeightField(meshVertToGlm(meshVertex)){
}

std::vector<glm::vec3> FLOOF::HeightField::meshVertToGlm(std::vector<MeshVertex> meshvert) {

    std::vector<glm::vec3> output(meshvert.size());

    for(int i {0}; i < meshvert.size(); i++){
        output[i] = meshvert[i].Pos;
    }

    return output;
}
