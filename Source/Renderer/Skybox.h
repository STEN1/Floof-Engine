#pragma once
#include "Cubemap.h"

namespace FLOOF {
    class Skybox {
    public:
        inline static const std::vector<SimpleVertex> skyboxVertices = {
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

        Skybox(const std::array<std::string, 6> paths);
        Skybox(const std::string& path);
        void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
        ~Skybox();

        Cubemap m_Cubemap;
        VulkanBufferData m_VertexBuffer;
    };
}