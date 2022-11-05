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
    };

    class StaticMeshComponent {
    public:
        StaticMeshComponent() = default;
        StaticMeshComponent(const std::string& path);
        std::shared_ptr<std::vector<MeshData>> meshes{};
        std::shared_ptr<std::vector<Material>> materials{};
    };
}