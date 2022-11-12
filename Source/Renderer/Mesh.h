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
        uint32_t MaterialIndex{};
    };

    class StaticMeshComponent {
    public:
        StaticMeshComponent() = default;
        StaticMeshComponent(const std::string& path);
        std::vector<MeshData> meshes{};
        std::vector<Material> materials{};
        std::unordered_map<std::string, bool> mapDrawWireframeMeshes{};
    };
}