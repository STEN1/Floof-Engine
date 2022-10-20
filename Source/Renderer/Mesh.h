#include "../Floof.h"
#include "Vertex.h"
#include "VulkanRenderer.h"

namespace FLOOF {
    struct MeshData {
        VulkanBuffer VertexBuffer{};
        VulkanBuffer IndexBuffer{};
        uint32_t VertexCount{};
        uint32_t IndexCount{};
    };

    template<typename VertexType>
    class StaticMesh {
    public:
        StaticMesh() = default;
        std::vector<MeshData> Data{};
        std::string Path;
    };

    template<typename VertexType>
    class DynamicMesh {
    public:
        DynamicMesh() = default;
        MeshData Data{};
    };

    using ModelMesh = StaticMesh<MeshVertex>;
    using StaticLineMeshComponent = StaticMesh<ColorVertex>;
    using DynamicLineMeshComponent = DynamicMesh<ColorVertex>;
}