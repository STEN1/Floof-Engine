#pragma once

#include "../Floof.h"
#include "Vertex.h"
#include "VulkanRenderer.h"
#include "Material.h"

namespace FLOOF {
    struct MeshData {
        VulkanBufferData VertexBuffer{};
        VulkanBufferData IndexBuffer{};
        uint32_t VertexCount{};
        uint32_t IndexCount{};
        std::string MeshName{"NoName"};

        Material MeshMaterial;
    };

    class StaticMeshComponent {
    public:
        StaticMeshComponent() = default;
        StaticMeshComponent(const std::string& path);
        StaticMeshComponent(const std::string& path, bool textureFromPath);
        std::vector<MeshData> meshes{};
        std::unordered_map<std::string, bool> mapDrawWireframeMeshes{};
    };
}