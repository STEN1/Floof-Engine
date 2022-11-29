#pragma once

#include "../Floof.h"
#include "Vertex.h"
#include "VulkanRenderer.h"
#include "Material.h"
#include "Mesh.h"

namespace FLOOF {
    struct LandscapeMesh {
        VulkanBufferData VertexBuffer{};
        VulkanBufferData IndexBuffer{};
        uint32_t VertexCount{};
        uint32_t IndexCount{};
        std::string MeshName{ "NoName" };

        LandscapeMaterial MeshMaterial1;
        LandscapeMaterial MeshMaterial2;
        LandscapeMaterial MeshMaterial3;
        Texture BlendTex;

        void setMesh(MeshData meshData){
            IndexBuffer = meshData.IndexBuffer;
            VertexBuffer = meshData.VertexBuffer;
            IndexCount = meshData.IndexCount;
            VertexCount = meshData.VertexCount;
        }
    };

    class StaticLandscapeMeshComponent {
    public:
        StaticLandscapeMeshComponent() = default;
        std::vector<LandscapeMesh> meshes{};
    };
}