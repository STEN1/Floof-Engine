#include "LasLoader.h"
#include <fstream>
#include <sstream>
#include <limits>
#include <stdio.h>
LasLoader::LasLoader(const std::string& path) : PointData{} {

    std::string txt(".txt");
    std::string lasbin(".lasbin");
    std::string las(".las");

    if (path.find(txt) != std::string::npos)
        ReadTxt(path);
    else if (path.find(lasbin) != std::string::npos)
        ReadBin(path);
    else if (path.find(las) != std::string::npos)
        ReadLas(path);
    // TODO: Dont calc center when reading .las (already in header)

    CalcCenter();
    UpdatePoints();
    Triangulate();
}

std::vector<FLOOF::ColorVertex> LasLoader::GetPointData() {
    return LasLoader::PointData;
}


void LasLoader::FindMinMax() {

    // Check if min/max already found
    if (max != glm::vec3(0.f)) {
        return;
    }
    min = glm::vec3(std::numeric_limits<float>::max());
    max = glm::vec3(std::numeric_limits<float>::min());

    for (auto& vertex : PointData) {
        if (min.x > vertex.Pos.x) { min.x = vertex.Pos.x; } else if (max.x < vertex.Pos.x) { max.x = vertex.Pos.x; }
        if (min.z > vertex.Pos.z) { min.z = vertex.Pos.z; } else if (max.z < vertex.Pos.z) { max.z = vertex.Pos.z; }
        if (min.y > vertex.Pos.y) { min.y = vertex.Pos.y; } else if (max.y < vertex.Pos.y) { max.y = vertex.Pos.y; }
    }
}

void LasLoader::CalcCenter() {

    FindMinMax();

    // Find middle
    middle = min + (max - min) / 2.f;
    offset = min;

    // Update min/max
    min -= offset;
    max -= offset;
}

void LasLoader::UpdatePoints() {

    for (auto& vertex : PointData) {
        if (middle != glm::vec3(0.f))
            vertex.Pos -= offset;
    }
}

void LasLoader::Triangulate() {

    // width and height
    xSquares = (max.x - min.x);
    zSquares = (max.z - min.z);

    // Save all height data for each vertex
    std::vector<std::vector<HeightAndColor>> heightmap(zSquares, std::vector<HeightAndColor>(xSquares));
    for (auto& vertex : PointData) {
        int xPos = vertex.Pos.x;
        int zPos = vertex.Pos.z;

        if (xPos < 0.f || xPos > xSquares - 1
            || zPos < 0.f || zPos > zSquares - 1) {
            continue;
        }

        // Instead of push back, add height and increment
        heightmap[zPos][xPos].count++;
        heightmap[zPos][xPos].sum += vertex.Pos.y;
        heightmap[zPos][xPos].color += vertex.Color;
    }

    std::vector<std::pair<int, int>> noHeight;

    // Calculate average height for each vertex and push
    for (int z = 0; z < zSquares; ++z) {
        for (int x = 0; x < xSquares; ++x) {
            float y;
            glm::vec3 color{};
            if (heightmap[z][x].count == 0) {
                y = -max.y;
                noHeight.push_back(std::make_pair(x, z));
                color = glm::vec3(1.f);
            } else {
                //y = (average / count) - max.y;
                y = heightmap[z][x].sum / heightmap[z][x].count - max.y;
                color = heightmap[z][x].color / glm::vec3(heightmap[z][x].count);
            }
            FLOOF::MeshVertex temp{};
            FLOOF::ColorNormalVertex temp2{};
            temp.Pos = glm::vec3(x, y, z);
            temp2.Pos = temp.Pos;
            temp2.Color = color;
            VertexData.push_back(temp);
            ColorNormalVertexData.push_back(temp2);
        }
    }

    // Calculate average height if no height
    for (auto& vertex : noHeight) {
        int x = vertex.first;
        int z = vertex.second;
        if (x <= 0 || x >= xSquares - 1 || z <= 0 || z >= zSquares - 1) {
            continue;
        } else {
            float averageHeight{ 0.f };
            averageHeight += VertexData[(x - 1) + (z * xSquares)].Pos.y;
            averageHeight += VertexData[(x - 1) + ((z - 1) * xSquares)].Pos.y;
            averageHeight += VertexData[x + ((z - 1) * xSquares)].Pos.y;
            averageHeight += VertexData[(x + 1) + ((z - 1) * xSquares)].Pos.y;
            averageHeight += VertexData[(x + 1) + (z * xSquares)].Pos.y;
            averageHeight += VertexData[(x + 1) + ((z + 1) * xSquares)].Pos.y;
            averageHeight += VertexData[x + ((z + 1) * xSquares)].Pos.y;
            averageHeight += VertexData[(x - 1) + ((z + 1) * xSquares)].Pos.y;

            glm::vec3 averageColor{};
            averageColor += ColorNormalVertexData[(x - 1) + (z * xSquares)].Color;
            averageColor += ColorNormalVertexData[(x - 1) + ((z - 1) * xSquares)].Color;
            averageColor += ColorNormalVertexData[x + ((z - 1) * xSquares)].Color;
            averageColor += ColorNormalVertexData[(x + 1) + ((z - 1) * xSquares)].Color;
            averageColor += ColorNormalVertexData[(x + 1) + (z * xSquares)].Color;
            averageColor += ColorNormalVertexData[(x + 1) + ((z + 1) * xSquares)].Color;
            averageColor += ColorNormalVertexData[x + ((z + 1) * xSquares)].Color;
            averageColor += ColorNormalVertexData[(x - 1) + ((z + 1) * xSquares)].Color;

            VertexData[x + (z * xSquares)].Pos.y = averageHeight / 8.f;
            ColorNormalVertexData[x + (z * xSquares)].Pos.y = averageHeight / 8.f;
            ColorNormalVertexData[x + (z * xSquares)].Color = averageColor / 8.f;
        }
    }

    // Create Index
    for (int z = 0; z < zSquares - 1; ++z) {
        for (int x = 0; x < xSquares - 1; ++x) {
            IndexData.emplace_back(x + (xSquares * z));
            IndexData.emplace_back(x + 1 + (xSquares * (z + 1)));
            IndexData.emplace_back(x + 1 + (xSquares * z));

            IndexData.emplace_back(x + (xSquares * z));
            IndexData.emplace_back(x + (xSquares * (z + 1)));
            IndexData.emplace_back(x + 1 + (xSquares * (z + 1)));
        }
    }

    // Calculate smooth normals
    int xmin = 0, xmax = xSquares, zmin = 0, zmax = zSquares; // The size to draw

    // Normals for rest
    for (int z = zmin; z < zmax; z++) {
        for (int x = xmin; x < xmax; x++) {
            if (z == zmin || z == zmax - 1 || x == xmin || x == xmax - 1) {
                VertexData[x + (xmax * z)].Normal = glm::vec3(0.f, 1.f, 0.f);
            } else {
                glm::vec3 a(VertexData[x + (xmax * z)].Pos);
                glm::vec3 b(VertexData[x + 1 + (xmax * z)].Pos);
                glm::vec3 c(VertexData[x + 1 + (xmax * (z + 1))].Pos);
                glm::vec3 d(VertexData[x + (xmax * (z + 1))].Pos);
                glm::vec3 e(VertexData[x - 1 + (xmax * z)].Pos);
                glm::vec3 f(VertexData[x - 1 + (xmax * (z - 1))].Pos);
                glm::vec3 g(VertexData[x + (xmax * (z - 1))].Pos);

                auto n0 = glm::cross(c - a, b - a);
                auto n1 = glm::cross(d - a, c - a);
                auto n2 = glm::cross(e - a, d - a);
                auto n3 = glm::cross(f - a, e - a);
                auto n4 = glm::cross(g - a, f - a);
                auto n5 = glm::cross(b - a, g - a);

                glm::vec3 normal = n0 + n1 + n2 + n3 + n4 + n5;
                normal = glm::normalize(normal);

                VertexData[x + (xmax * z)].Normal = normal;
                ColorNormalVertexData[x + (xmax * z)].Normal = normal;
            }
        }
    }
}


std::pair<std::vector<FLOOF::MeshVertex>, std::vector<uint32_t>> LasLoader::GetIndexedData() {
    return { LasLoader::VertexData, LasLoader::IndexData };
}

std::vector<FLOOF::MeshVertex> LasLoader::GetVertexData() {
    std::vector<FLOOF::MeshVertex> out;
    int i = 0;
    while (i != IndexData.size()) {
        FLOOF::MeshVertex tempA;
        tempA = VertexData[IndexData[i]];
        i++;

        FLOOF::MeshVertex tempB;
        tempB = VertexData[IndexData[i]];
        i++;

        FLOOF::MeshVertex tempC;
        tempC = VertexData[IndexData[i]];
        i++;

        auto ab = tempB.Pos - tempA.Pos;
        auto ac = tempC.Pos - tempA.Pos;

        glm::vec3 normal = glm::normalize(glm::cross(ab, ac));
        tempA.Normal = normal;
        tempB.Normal = normal;
        tempC.Normal = normal;

        tempA.UV.x = tempA.Pos.x;
        tempA.UV.y = tempA.Pos.y;
        tempB.UV.x = tempB.Pos.x;
        tempB.UV.y = tempB.Pos.y;
        tempC.UV.x = tempC.Pos.x;
        tempC.UV.y = tempC.Pos.y;

        out.push_back(tempA);
        out.push_back(tempB);
        out.push_back(tempC);
    }
    return out;
}

std::pair<std::vector<FLOOF::ColorNormalVertex>, std::vector<uint32_t>> LasLoader::GetIndexedColorNormalVertexData() {
    return { ColorNormalVertexData, IndexData };
}


void LasLoader::ReadTxt(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "Cant open file: " << path << std::endl;
        return;
    }
    std::string line;
    FLOOF::ColorVertex tempVertex{};
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        ss >> tempVertex.Pos.x;
        ss >> tempVertex.Pos.z;
        ss >> tempVertex.Pos.y;
        tempVertex.Color = glm::vec3(0.f, 1.f, 0.f);
        PointData.push_back(tempVertex);
    }
}

void LasLoader::ReadBin(const std::string& path) {

    std::ifstream is(path, std::ios::binary | std::ios::ate);
    auto size = is.tellg();
    std::vector<glm::vec3> lasDataPoints(size / sizeof(glm::vec3));
    is.seekg(0);
    is.read(reinterpret_cast<char*>(lasDataPoints.data()), size);

    for (auto& point : lasDataPoints) {
        FLOOF::ColorVertex tempVertex{};
        tempVertex.Pos.x = point.x;
        tempVertex.Pos.y = point.z;
        tempVertex.Pos.z = point.y;
        tempVertex.Color = glm::vec3(1.f, 1.f, 1.f);
        PointData.push_back(tempVertex);
    }
}

void LasLoader::ReadLas(const std::string& path) {
    lasHeader header;
    std::ifstream inf(path, std::ios::binary | std::ios::ate);
    if (inf.is_open()) {

        // Read Header
        inf.seekg(0);
        inf.read((char*)&header.fileSignature, sizeof(header.fileSignature));
        inf.read((char*)&header.sourceID, sizeof(header.sourceID));
        inf.read((char*)&header.globalEncoding, sizeof(header.globalEncoding));
        inf.read((char*)&header.GUID1, sizeof(header.GUID1));
        inf.read((char*)&header.GUID2, sizeof(header.GUID2));
        inf.read((char*)&header.GUID3, sizeof(header.GUID3));
        inf.read((char*)&header.GUID4, sizeof(header.GUID4));
        inf.read((char*)&header.versionMajor, sizeof(header.versionMajor));
        inf.read((char*)&header.versionMinor, sizeof(header.versionMinor));
        inf.read((char*)&header.systemIdentifier, sizeof(header.systemIdentifier));
        inf.read((char*)&header.generatingSoftware, sizeof(header.generatingSoftware));
        inf.read((char*)&header.creationDay, sizeof(header.creationDay));
        inf.read((char*)&header.creationYear, sizeof(header.creationYear));
        inf.read((char*)&header.headerSize, sizeof(header.headerSize));
        inf.read((char*)&header.offsetToPointData, sizeof(header.offsetToPointData));
        inf.read((char*)&header.numberVariableLengthRecords, sizeof(header.numberVariableLengthRecords));
        inf.read((char*)&header.pointDataRecordFormat, sizeof(header.pointDataRecordFormat));
        inf.read((char*)&header.pointDataRecordLength, sizeof(header.pointDataRecordLength));
        inf.read((char*)&header.legacyNumberPointsRecords, sizeof(header.legacyNumberPointsRecords));
        inf.read((char*)&header.legacyNumberPointReturn, sizeof(header.legacyNumberPointReturn));
        inf.read((char*)&header.xScaleFactor, sizeof(header.xScaleFactor));
        inf.read((char*)&header.yScaleFactor, sizeof(header.yScaleFactor));
        inf.read((char*)&header.zScaleFactor, sizeof(header.zScaleFactor));
        inf.read((char*)&header.xOffset, sizeof(header.xOffset));
        inf.read((char*)&header.yOffset, sizeof(header.yOffset));
        inf.read((char*)&header.zOffset, sizeof(header.zOffset));
        inf.read((char*)&header.maxX, sizeof(header.maxX));
        inf.read((char*)&header.minX, sizeof(header.minX));
        inf.read((char*)&header.maxY, sizeof(header.maxY));
        inf.read((char*)&header.minY, sizeof(header.minY));
        inf.read((char*)&header.maxZ, sizeof(header.maxZ));
        inf.read((char*)&header.minZ, sizeof(header.minZ));

        // Save max and min from header (so that we don't need to calculate it later)
        min.x = header.minX;
        min.y = header.minZ;
        min.z = header.minY;
        max.x = header.maxX;
        max.y = header.maxZ;
        max.z = header.maxY;

        // Read pointdata
        inf.seekg(header.offsetToPointData);

        // Only format 1 and 2 supported
        ASSERT(header.pointDataRecordFormat == 1 || header.pointDataRecordFormat == 2);

        // Read format 1
        if (header.pointDataRecordFormat == 1) {
            for (int i = 0; i < header.legacyNumberPointsRecords; ++i) {
                lasPointData1 temp;
                inf.seekg(header.offsetToPointData + (header.pointDataRecordLength * i));
                inf.read((char*)&temp.xPos, sizeof(temp.xPos));
                inf.read((char*)&temp.yPos, sizeof(temp.yPos));
                inf.read((char*)&temp.zPos, sizeof(temp.zPos));

                FLOOF::ColorVertex tempVertex{};
                // Final position = (pos * scale factor) + offset
                tempVertex.Pos.x = (temp.xPos * header.xScaleFactor) + header.xOffset;
                tempVertex.Pos.y = (temp.zPos * header.zScaleFactor) + header.zOffset;
                tempVertex.Pos.z = (temp.yPos * header.yScaleFactor) + header.yOffset;
                tempVertex.Color = glm::vec3(0.f, 1.f, 0.f);
                PointData.push_back(tempVertex);
            }
        }
        // Read format 2
        else if (header.pointDataRecordFormat == 2) {
            for (int i = 0; i < header.legacyNumberPointsRecords; ++i) {
                lasPointData2 temp;
                inf.read((char*)&temp.xPos, sizeof(temp.xPos));
                inf.read((char*)&temp.yPos, sizeof(temp.yPos));
                inf.read((char*)&temp.zPos, sizeof(temp.zPos));
                inf.read((char*)&temp.intensity, sizeof(temp.intensity));
                inf.read((char*)&temp.flags, sizeof(temp.flags));
                inf.read((char*)&temp.classificaton, sizeof(temp.classificaton));
                inf.read((char*)&temp.scanAngle, sizeof(temp.scanAngle));
                inf.read((char*)&temp.userData, sizeof(temp.userData));
                inf.read((char*)&temp.pointSourceID, sizeof(temp.pointSourceID));
                inf.read((char*)&temp.red, sizeof(temp.red));
                inf.read((char*)&temp.green, sizeof(temp.green));
                inf.read((char*)&temp.blue, sizeof(temp.blue));

                FLOOF::ColorVertex tempVertex{};
                tempVertex.Pos.x = (temp.xPos * header.xScaleFactor) + header.xOffset;
                tempVertex.Pos.y = (temp.zPos * header.zScaleFactor) + header.zOffset;
                tempVertex.Pos.z = (temp.yPos * header.yScaleFactor) + header.yOffset;
                tempVertex.Color = glm::vec3(temp.red * 0.00001, temp.green * 0.00001, temp.blue * 0.00001);
                PointData.push_back(tempVertex);
            }
        }
    }
}
