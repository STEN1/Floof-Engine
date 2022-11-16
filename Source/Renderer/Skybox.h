#pragma once
#include "Cubemap.h"

namespace FLOOF {
    class Skybox {
    public:
        Skybox(const std::array<std::string, 6> paths);
        void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
        ~Skybox();
    private:
        Cubemap m_Cubemap;
        VulkanBufferData m_VertexBuffer;
    };
}