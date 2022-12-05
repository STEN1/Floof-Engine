#ifndef FLOOF_LINEMESHCOMPONENT_H
#define FLOOF_LINEMESHCOMPONENT_H

#include "Renderer/VulkanRenderer.h"
#include "Renderer/Vertex.h"

namespace FLOOF {


    struct LineMeshComponent {
        LineMeshComponent(const std::vector<ColorVertex> &vertexData);

        ~LineMeshComponent() {
            auto renderer = VulkanRenderer::Get();
            if (VertexBuffer.Buffer != VK_NULL_HANDLE)
                vmaDestroyBuffer(renderer->m_Allocator, VertexBuffer.Buffer, VertexBuffer.Allocation);
        }

        void Draw(VkCommandBuffer commandBuffer) {
            if (VertexCount == 0)
                return;

            VkDeviceSize offset{0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VertexBuffer.Buffer, &offset);
            vkCmdDraw(commandBuffer, VertexCount, 1, 0, 0);
        }

        void UpdateBuffer(const std::vector<ColorVertex> &vertexData) {
            VertexCount = vertexData.size();
            if (VertexCount > MaxVertexCount) {
                VertexCount = MaxVertexCount;
            }
            // Buffer is already mapped. You can access its memory.
            memcpy(VertexBuffer.AllocationInfo.pMappedData, vertexData.data(), sizeof(ColorVertex) * VertexCount);
        }

        VulkanBufferData VertexBuffer{};
        uint32_t VertexCount{};
        uint32_t MaxVertexCount{};
    };
}
#endif //FLOOF_LINEMESHCOMPONENT_H
