#include "LineMeshComponent.h"
#include "Renderer/VulkanRenderer.h"
#include "Renderer/Vertex.h"

namespace FLOOF {
    LineMeshComponent::LineMeshComponent(const std::vector<ColorVertex> &vertexData) {
        auto renderer = VulkanRenderer::Get();

        VertexCount = vertexData.size();
        MaxVertexCount = VertexCount;
        VkBufferCreateInfo bufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufCreateInfo.size = sizeof(ColorVertex) * VertexCount;
        bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                VMA_ALLOCATION_CREATE_MAPPED_BIT;

        vmaCreateBuffer(renderer->m_Allocator, &bufCreateInfo, &allocCreateInfo, &VertexBuffer.Buffer,
                        &VertexBuffer.Allocation, &VertexBuffer.AllocationInfo);

        // Buffer is already mapped. You can access its memory.
        memcpy(VertexBuffer.AllocationInfo.pMappedData, vertexData.data(), sizeof(ColorVertex) * VertexCount);
    }
}
