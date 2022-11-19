#include "Skybox.h"

namespace FLOOF {
    Skybox::Skybox(const std::array<std::string, 6> paths)
        :m_Cubemap(paths)
    {
        auto* renderer = VulkanRenderer::Get();
        m_VertexBuffer = renderer->CreateVertexBuffer(skyboxVertices);
    }
    Skybox::Skybox(const std::string& path)
        :m_Cubemap(path)
    {
        auto* renderer = VulkanRenderer::Get();
        m_VertexBuffer = renderer->CreateVertexBuffer(skyboxVertices);
    }
    void Skybox::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
            &m_Cubemap.CubemapTexture.DesctriptorSet, 0, nullptr);
        VkDeviceSize offset{ 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer.Buffer, &offset);
        vkCmdDraw(commandBuffer, skyboxVertices.size(), 1, 0, 0);
    }
    Skybox::~Skybox()
    {
        auto* renderer = VulkanRenderer::Get();
        renderer->FinishAllFrames();
        renderer->DestroyVulkanBuffer(&m_VertexBuffer);
    }
}