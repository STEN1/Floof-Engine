#include "Skybox.h"

namespace FLOOF {
    static const std::vector<SimpleVertex> skyboxVertices = {
        // positions          
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},
        SimpleVertex{{-1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f,  1.0f, -1.0f}},
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},

        SimpleVertex{{-1.0f, -1.0f,  1.0f}},
        SimpleVertex{{-1.0f, -1.0f, -1.0f}},
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},
        SimpleVertex{{-1.0f,  1.0f,  1.0f}},
        SimpleVertex{{-1.0f, -1.0f,  1.0f}},

        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},

        SimpleVertex{{-1.0f, -1.0f,  1.0f}},
        SimpleVertex{{-1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f, -1.0f,  1.0f}},
        SimpleVertex{{-1.0f, -1.0f,  1.0f}},

        SimpleVertex{{-1.0f,  1.0f, -1.0f}},
        SimpleVertex{{1.0f,  1.0f, -1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{1.0f,  1.0f,  1.0f}},
        SimpleVertex{{-1.0f,  1.0f,  1.0f}},
        SimpleVertex{{-1.0f,  1.0f, -1.0f}},

        SimpleVertex{{-1.0f, -1.0f, -1.0f}},
        SimpleVertex{{-1.0f, -1.0f,  1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{1.0f, -1.0f, -1.0f}},
        SimpleVertex{{-1.0f, -1.0f,  1.0f}},
        SimpleVertex{{1.0f, -1.0f,  1.0}}
    };

    Skybox::Skybox(const std::array<std::string, 6> paths)
        :m_Cubemap(paths)
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