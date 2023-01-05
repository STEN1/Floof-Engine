#include "HeightmapLoader.h"
#include <iostream>
#include "glm/glm.hpp"
#include "Renderer/VulkanRenderer.h"
#include "Renderer/Mesh.h"

namespace FLOOF {

    HeightmapLoader::HeightmapLoader(const char *mapstr) : filepath(mapstr) {
        if (readMap()) 
            std::cout << "heightmap sucsses\n";
    }

    bool HeightmapLoader::readMap() {

        img = stbi_load(filepath, &width, &height, &channels, 0);
        if (img) {
            std::cout << "Loaded heigtmap image \n\twidth: " << width << "\tHeight: " << height << ", \tChannels: " << channels;
            for (int x = 0; x < height; x++) {
                for (int y = 0; y < width; y++) {
                    float z = img[x * width + y] * zScale;
                    mVertices.emplace_back(MeshVertex{ glm::vec3(x, z, y) });
                }
            }

            //set mIndices
            for (int x = 0; x < height - 1; x++) {
                for (int y = 0; y < width - 1; y++) {

                    auto a{ x * width + y };
                    auto b{ (x + 1) * width + y };
                    auto c{ (x + 1) * width + (y + 1) };
                    auto d{ x * width + (y + 1) };

                    mIndices.emplace_back(a);
                    mIndices.emplace_back(c);
                    mIndices.emplace_back(b);

                    mIndices.emplace_back(a);
                    mIndices.emplace_back(d);
                    mIndices.emplace_back(c);
                }
            }

            //set Normals
            for (int x = 1; x < height - 1; x++) {
                for (int y = 1; y < width - 1; y++) {
                    glm::vec3 a = mVertices[(x * width) + y].Pos;
                    glm::vec3 b = mVertices[((x + 1) * width) + y].Pos;
                    glm::vec3 c = mVertices[((x + 1) * width) + y + 1].Pos;
                    glm::vec3 d = mVertices[(x * width) + y + 1].Pos;
                    glm::vec3 e = mVertices[((x - 1) * width) + y].Pos;
                    glm::vec3 f = mVertices[(x * width) + (y - 1)].Pos;
                    glm::vec3 g = mVertices[((x - 1) * width) + (y - 1)].Pos;

                    auto a0 = glm::cross(f - a, b - a);
                    auto a1 = glm::cross(b - a, d - a);
                    auto a2 = glm::cross(d - a, c - a);
                    auto a3 = glm::cross(c - a, e - a);
                    auto a4 = glm::cross(e - a, g - a);
                    auto a5 = glm::cross(g - a, f - a);

                    glm::vec3 normal = glm::normalize(a0 + a1 + a2 + a3 + a4 + a5);
                    mVertices[(x * height) + y].Normal = normal * glm::vec3{ -1, -1, -1 };

                    //uv
                    //auto UVx = (1.f / height) * y;
                    //auto UVy = (1.f / width) * x;
                    auto UVx = (0.1f / height) * y;
                    auto UVy = (0.1f / width) * x;

                    mVertices[(x * width) + y].UV = glm::vec2(UVx, UVy);
                }
            }
            for (size_t i = 0; i < 3; i++){
                for (int x = 0; x < height - 1; x++) {
                    for (int y = 0; y < width - 1; y++) {
                        mVertices[(x * width) + y].Pos.y = getAvearageHeight(x, y);
                    }
                }
            }
            stbi_image_free(img);      //free image memory after bitmap generation
        } else {
            stbi_image_free(img);      //free image memory after bitmap generation
            std::cout << "Heightmap failed to read or could not find the file in the librariy";
            return false;
        }
        return true;
    }

    void HeightmapLoader::MakeBuffersFromData(const std::vector<MeshVertex> &meshData,
                                              const std::vector<uint32_t> indexData) {
        auto *renderer = VulkanRenderer::Get();
        auto vertexBuffer = renderer->CreateVertexBuffer(meshData);
        auto indexBuffer = renderer->CreateIndexBuffer(indexData);
        auto vertexBuffffer = vertexBuffer.Buffer;
    }

    MeshData HeightmapLoader::getMeshData() {
        MeshData retrn;

        auto *renderer = VulkanRenderer::Get();
        retrn.VertexBuffer = renderer->CreateVertexBuffer(mVertices);
        retrn.IndexBuffer = renderer->CreateIndexBuffer(mIndices);
        retrn.IndexCount = mIndices.size();
        retrn.VertexCount = mVertices.size();
        return retrn;
    }

    float HeightmapLoader::getAvearageHeight(int x, int z)
    {
        std::vector<float> all_Heigts;

        if ((x + 1) * width < width * height) {
            all_Heigts.push_back(mVertices[((x + 1) * width) + z].Pos.y);

            if ((x * width) + z + 1 < (width * height))
                all_Heigts.push_back(mVertices[((x + 1) * width) + z + 1].Pos.y);
            if ((x*width) + z - 1 > 0)
                all_Heigts.push_back(mVertices[((x + 1) * width) + (z - 1)].Pos.y);
        }

        if ((x * width) + z + 1 < width * height) {
            all_Heigts.push_back(mVertices[(x * width) + z + 1].Pos.y);
        }

        if ((x - 1) * width > 0) {
            all_Heigts.push_back(mVertices[((x - 1) * width) + z].Pos.y);

            if ((x * width) + z - 1 > 0)
                all_Heigts.push_back(mVertices[((x - 1) * width) + (z - 1)].Pos.y);

            if ((x * width) + z + 1 < width * height)
                all_Heigts.push_back(mVertices[((x - 1) * width) + z].Pos.y);
        }

        if ((x * width) + z - 1 > 0)
            all_Heigts.push_back(mVertices[(x * width) + (z - 1)].Pos.y);

        float average{ 0 };
        int am{ 0 };

        if (!all_Heigts.empty()) {
            for (auto a : all_Heigts)
            {
                average += a;
                am++;
            }
            return average / am;
        }
        else return 0;
    }

    float HeightmapLoader::getHeight(const glm::vec2& pos)
    {
        glm::vec2 PlayerPosCopy = pos;
        PlayerPosCopy.x = floorf(PlayerPosCopy.x);
        PlayerPosCopy.y = floorf(PlayerPosCopy.y);

        int a1 = (PlayerPosCopy.y * width) + PlayerPosCopy.x;
        int a2 = (PlayerPosCopy.y + 1) * width + PlayerPosCopy.x + 1;
        int a3 = (PlayerPosCopy.y + 1) * width + PlayerPosCopy.x;
        int a4 = (PlayerPosCopy.y * width) + PlayerPosCopy.x + 1;

        if (a1 > mVertices.size() || a2 > mVertices.size() || a3 > mVertices.size() || a4 > mVertices.size() || a1 < 0 || a2 < 0 || a3 < 0 || a4 < 0)
            return 0;

        glm::vec2 a = glm::vec2(mVertices[a1].Pos.x, mVertices[a1].Pos.y);
        glm::vec2 b = glm::vec2(mVertices[a2].Pos.x, mVertices[a2].Pos.y);
        glm::vec2 c = glm::vec2(mVertices[a3].Pos.x, mVertices[a3].Pos.y);
        glm::vec2 d = glm::vec2(mVertices[a4].Pos.x, mVertices[a4].Pos.y);

        glm::vec3 baryLocation = barysentricCordinatats(pos, a, d, b);

        bool btri{ true };
        int i{ 0 };

        while (btri && i > 3)
        {
            if (baryLocation[i] < 0.f || baryLocation[i] > 1.f)
            {
                btri = false;
            }
            i++;
        }

        if (btri) {
            float ah = mVertices[a1].Pos.z * baryLocation.x;
            float bh = mVertices[a4].Pos.z * baryLocation.y;
            float ch = mVertices[a2].Pos.z * baryLocation.z;
            float height = ah + bh + ch;
            return height;
        }
        else {
            baryLocation = barysentricCordinatats(pos, a, b, c);

            float ah = mVertices[a1].Pos.z * baryLocation.x;
            float bh = mVertices[a2].Pos.z * baryLocation.y;
            float dh = mVertices[a3].Pos.z * baryLocation.z;
            float height = ah + bh + dh;
            return height;
        }
    }

    glm::vec3 HeightmapLoader::barysentricCordinatats(const glm::vec2& x, glm::vec2& p1, glm::vec2& p2, glm::vec2& p3)
    {
        glm::vec2 p12 = p2 - p1;
        glm::vec2 p13 = p3 - p1;
        glm::vec3 n{ crossProduct(p12 , p13) };
        float areal_123 = n.length(); // dobbelt areal
        glm::vec3 baryc; // til retur. Husk

        //u
        glm::vec2 p = p2 - x;
        glm::vec2 q = p3 - x;
        n = crossProduct(p, q);
        baryc.x = n.z / areal_123;
        //v
        p = p3 - x;
        q = p1 - x;
        n = crossProduct(p, q);
        baryc.y = n.z / areal_123;
        //w
        p = p1 - x;
        q = p2 - x;
        n = crossProduct(p, q);
        baryc.z = n.z / areal_123;
        return baryc;
    }

    glm::vec3 HeightmapLoader::crossProduct(glm::vec2 a, glm::vec2 b)
    {
        return glm::vec3{ a.y * 0 - 0 * b.y,0 * b.x - a.x * 0 ,a.x * b.y - a.y * b.x };
    }
}