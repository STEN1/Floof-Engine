#include "Utils.h"

#include "Floof.h"

#include <fstream>
#include <string>
#include <sstream>

#include "LoggerMacros.h"
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

        glm::vec3 CalcBarycentric(glm::vec3 position, const Triangle& triangle) {
            glm::vec2 pos{ position.x, position.z };

            glm::vec2 p1 = glm::vec2(triangle.A.x, triangle.A.z);
            glm::vec2 p2 = glm::vec2(triangle.B.x, triangle.B.z);
            glm::vec2 p3 = glm::vec2(triangle.C.x, triangle.C.z);

            glm::vec2 Va = p2 - p1;
            glm::vec2 Vb = p3 - p1;
            glm::vec3 n = glm::cross(glm::vec3(Va.x, Va.y, 0), glm::vec3(Vb.x, Vb.y, 0));
            float area = glm::length(n);
            glm::vec3 cords;

            glm::vec2 p = p2 - pos;
            glm::vec2 q = p3 - pos;
            n = glm::cross(glm::vec3(q.x, q.y, 0), glm::vec3(p.x, p.y, 0));
            cords.x = n.z / area;

            p = p3 - pos;
            q = p1 - pos;
            n = glm::cross(glm::vec3(q.x, q.y, 0), glm::vec3(p.x, p.y, 0));
            cords.y = n.z / area;

            p = p1 - pos;
            q = p2 - pos;
            n = glm::cross(glm::vec3(q.x, q.y, 0), glm::vec3(p.x, p.y, 0));
            cords.z = n.z / area;

            return cords;
        }
        bool IsPointInsideTriangle(const glm::vec3& position, const Triangle& triangle) {
            glm::vec3 v = Utils::CalcBarycentric(position, triangle);
            for (size_t i = 0; i < 3; i++) {
                if (v[i] < 0.f || v[i] > 1.f)
                    return false;
            }
            return true;
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

        std::vector<Triangle> GetVisimTriangles(const std::string& path) {

            std::ifstream file(path);
            if (!file.is_open()) {
                std::string msg = path;
                msg += " could not open";
                LOG_ERROR(msg.c_str());
                return {};
            }
            std::string line;
            std::getline(file, line);
            std::stringstream ss(line);
            int tricount;
            ss >> tricount;
            tricount = tricount / 3;
            std::vector<Triangle> triangles(tricount);

            auto calcNormal = [](Triangle& triangle) {
                auto ab = triangle.A - triangle.B;
                auto ac = triangle.A - triangle.C;
                triangle.N = glm::normalize(glm::cross(ab, ac));
            };

            for (auto& tri : triangles) {
                {
                    std::string line;
                    std::getline(file, line);
                    std::stringstream ss(line);
                    ss >> tri.A.x;
                    ss >> tri.A.y;
                    ss >> tri.A.z;
                }
                {
                    std::string line;
                    std::getline(file, line);
                    std::stringstream ss(line);
                    ss >> tri.B.x;
                    ss >> tri.B.y;
                    ss >> tri.B.z;
                }
                {
                    std::string line;
                    std::getline(file, line);
                    std::stringstream ss(line);
                    ss >> tri.C.x;
                    ss >> tri.C.y;
                    ss >> tri.C.z;

                    ss >> tri.FrictionConstant;
                }

                calcNormal(tri);
            }


            return triangles;
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
    }
}

