#include "Utils.h"

#include "Floof.h"

#include <fstream>
#include <string>
#include <sstream>

#include "ObjLoader.h"


namespace FLOOF {
    namespace Utils {
        static void SubDivide(glm::vec3& a, glm::vec3& b, glm::vec3& c, int recursions, std::vector<FLOOF::MeshVertex>& vertexData, float radius);

        std::vector<MeshVertex> GetVisimVertexData(const std::string& path) {
            std::vector<MeshVertex> vertexData;

            std::ifstream file(path);
            if (!file.is_open()) {
                std::cout << "Cant open file: " << path << std::endl;
                return vertexData;
            }

            uint32_t vertexCount{};
            file >> vertexCount;
            std::string s;
            std::getline(file, s);
            vertexData.resize(vertexCount);
            for (MeshVertex& vertex : vertexData) {
                std::string line;
                std::getline(file, line);
                std::stringstream ss(line);
                ss >> vertex.Pos.x;
                ss >> vertex.Pos.y;
                ss >> vertex.Pos.z;
            }

            for (uint32_t i = 2; i < vertexData.size(); i += 3) {
                MeshVertex& a = vertexData[i - 2];
                MeshVertex& b = vertexData[i - 1];
                MeshVertex& c = vertexData[i - 0];

                glm::vec3 ab = b.Pos - a.Pos;
                glm::vec3 ac = c.Pos - a.Pos;

                glm::vec3 normal = glm::normalize(glm::cross(ab, ac));

                a.Normal = normal;
                b.Normal = normal;
                c.Normal = normal;

                a.UV.x = a.Pos.x;
                a.UV.y = a.Pos.y;
                b.UV.x = b.Pos.x;
                b.UV.y = b.Pos.y;
                c.UV.x = c.Pos.x;
                c.UV.y = c.Pos.y;
            }

            return vertexData;
        }

        std::vector<MeshVertex> MakeBall(int subdivisions, float radius) {
            std::vector<MeshVertex> vertexData;

            glm::vec3 v0{ 0,0,radius };
            glm::vec3 v1{ radius,0,0 };
            glm::vec3 v2{ 0,radius,0 };
            glm::vec3 v3{ -radius,0,0 };
            glm::vec3 v4{ 0,-radius,0 };
            glm::vec3 v5{ 0,0,-radius };

            SubDivide(v0, v1, v2, subdivisions, vertexData, radius);
            SubDivide(v0, v2, v3, subdivisions, vertexData, radius);
            SubDivide(v0, v3, v4, subdivisions, vertexData, radius);
            SubDivide(v0, v4, v1, subdivisions, vertexData, radius);
            SubDivide(v5, v2, v1, subdivisions, vertexData, radius);
            SubDivide(v5, v3, v2, subdivisions, vertexData, radius);
            SubDivide(v5, v4, v3, subdivisions, vertexData, radius);
            SubDivide(v5, v1, v4, subdivisions, vertexData, radius);

            return vertexData;
        }
        static void SubDivide(glm::vec3& a, glm::vec3& b, glm::vec3& c, int recursions, std::vector<FLOOF::MeshVertex>& vertexData, float radius) {
            if (recursions > 0) {
                glm::vec3 v1 = glm::normalize(a + b) * radius;
                glm::vec3 v2 = glm::normalize(a + c) * radius;
                glm::vec3 v3 = glm::normalize(c + b) * radius;

                SubDivide(a, v1, v2, recursions - 1, vertexData, radius);
                SubDivide(c, v2, v3, recursions - 1, vertexData, radius);
                SubDivide(b, v3, v1, recursions - 1, vertexData, radius);
                SubDivide(v3, v2, v1, recursions - 1, vertexData, radius);
            } else {
                glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
                FLOOF::MeshVertex v{};
                v.Pos = a;
                v.Normal = normal;
                vertexData.push_back(v);

                v.Pos = b;
                vertexData.push_back(v);

                v.Pos = c;
                vertexData.push_back(v);
            }
        }

        std::vector<ColorVertex> LineVertexDataFromObj(const std::string& path) {
            auto [vertexData, indexData] = ObjLoader(path).GetIndexedData();

            std::vector<ColorVertex> out(indexData.size());
            glm::vec3 color(1.f);

            for (size_t i = 0; i < indexData.size(); i++) {
                out[i].Pos = vertexData[indexData[i]].Pos;
                out[i].Color = color;
            }

            return out;
        }

        std::vector<ColorVertex> MakeBox(glm::vec3 extents, glm::vec3 color) {
            glm::vec3 a = glm::vec3(-extents.x, extents.y, -extents.z);
            glm::vec3 b = glm::vec3(extents.x, extents.y, -extents.z);
            glm::vec3 c = glm::vec3(extents.x, extents.y, extents.z);
            glm::vec3 d = glm::vec3(-extents.x, extents.y, extents.z);
            glm::vec3 e = glm::vec3(-extents.x, -extents.y, -extents.z);
            glm::vec3 f = glm::vec3(extents.x, -extents.y, -extents.z);
            glm::vec3 g = glm::vec3(extents.x, -extents.y, extents.z);
            glm::vec3 h = glm::vec3(-extents.x, -extents.y, extents.z);

            std::vector<ColorVertex> vertexData = {
                // Top
                // ab
                ColorVertex { a, color },
                ColorVertex { b, color },
                // bc
                ColorVertex { b, color },
                ColorVertex { c, color },
                // cd
                ColorVertex { c, color },
                ColorVertex { d, color },
                // da
                ColorVertex { d, color },
                ColorVertex { a, color },

                // Sides
                // ae
                ColorVertex { a, color },
                ColorVertex { e, color },
                // bf
                ColorVertex { b, color },
                ColorVertex { f, color },
                // cg
                ColorVertex { c, color },
                ColorVertex { g, color },
                // dh
                ColorVertex { d, color },
                ColorVertex { h, color },

                // Bottom
                // ef
                ColorVertex { e, color },
                ColorVertex { f, color },
                // fg
                ColorVertex { f, color },
                ColorVertex { g, color },
                // gh
                ColorVertex { g, color },
                ColorVertex { h, color },
                // he
                ColorVertex { h, color },
                ColorVertex { e, color },
            };
            return vertexData;
        }

        glm::vec3 btToglm(const btVector3 b) {
            return glm::vec3(b.x(),b.y(),b.z());
        }

        btVector3 glmTobt(const glm::vec3 b) {
            return btVector3(b.x,b.y,b.z);
        }
    }
}

