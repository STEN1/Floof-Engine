#pragma once
#include <string>
#include <vector>
#include "Floof.h"
#include "Vertex.h"
#include "Physics.h"
class LasLoader {

public:
    LasLoader(const std::string& path);
    std::vector<FLOOF::ColorVertex> GetPointData();
    std::pair<std::vector<FLOOF::MeshVertex>, std::vector<uint32_t>> GetIndexedData();
    std::vector<FLOOF::MeshVertex> GetVertexData();
    std::pair<std::vector<FLOOF::ColorNormalVertex>, std::vector<uint32_t>> GetIndexedColorNormalVertexData();
    std::vector<std::vector<std::pair<FLOOF::Triangle, FLOOF::Triangle>>> GetTerrainData();
    float GetMinY() { return -max.y; }
private:
    std::vector<FLOOF::ColorVertex> PointData;
    std::vector<FLOOF::MeshVertex> VertexData;
    std::vector<FLOOF::ColorNormalVertex> ColorNormalVertexData;
    std::vector<uint32_t> IndexData;
    std::vector<FLOOF::MeshVertex> TriangulatedVertexData;
    std::vector<FLOOF::Triangle> triangles;

    void ReadTxt(const std::string& path);
    void ReadBin(const std::string& path);
    void ReadLas(const std::string& path);

    void CalcCenter();
    void FindMinMax();
    void UpdatePoints();
    void Triangulate();

    glm::vec3 min{ 0.f };
    glm::vec3 max{ 0.f };
    glm::vec3 middle{ 0.f };
    glm::vec3 offset{ 0.f };
    int xSquares{ 0 };
    int zSquares{ 0 };
};

struct HeightAndColor {
    int count{ 0 };
    float sum{ 0.f };
    glm::vec3 color{ 0.f };
};

// Can't use struct directly because of padding of the size of the struct
struct lasHeader {
    char fileSignature[4];
    uint16_t sourceID;
    uint16_t globalEncoding;
    uint32_t GUID1;
    uint16_t GUID2;
    uint16_t GUID3;
    uint8_t GUID4[8];
    uint8_t versionMajor;
    uint8_t versionMinor;
    char systemIdentifier[32];
    char generatingSoftware[32];
    uint16_t creationDay;
    uint16_t creationYear;
    uint16_t headerSize;
    uint32_t offsetToPointData;
    uint32_t numberVariableLengthRecords;
    uint8_t pointDataRecordFormat;
    uint16_t pointDataRecordLength;
    int32_t legacyNumberPointsRecords;
    int32_t legacyNumberPointReturn[5];
    double xScaleFactor, yScaleFactor, zScaleFactor;
    double xOffset, yOffset, zOffset;
    double maxX, minX;
    double maxY, minY;
    double maxZ, minZ;
};

// Not needed
struct lasVariableLengthRecords {
    // Variable Length Records
    uint16_t lasReserved;
    char UserID[16];
    uint16_t recordID;
    uint16_t recordLengthAfterHeader;
    char lasDescription[32];
};

// Can't use struct directly because of padding of the size of the struct
struct lasPointData1 {
    int32_t xPos;
    int32_t yPos;
    int32_t zPos;
    uint16_t intensity;
    int8_t flags;
    uint8_t classificaton;
    int8_t scanAngle;
    uint8_t userData;
    uint16_t pointSourceID;
    double GPSTime;
};

// Can't use struct directly because of padding of the size of the struct
struct lasPointData2 {
    int32_t xPos;
    int32_t yPos;
    int32_t zPos;
    uint16_t intensity;
    int8_t flags;
    uint8_t classificaton;
    int8_t scanAngle;
    uint8_t userData;
    uint16_t pointSourceID;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
};


