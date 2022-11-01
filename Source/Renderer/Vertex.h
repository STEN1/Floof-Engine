#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

#include "../Math.h"

namespace FLOOF {
    struct MeshVertex {
        glm::vec3 Pos{};
        glm::vec3 Normal{};
        glm::vec2 UV{};
        glm::vec3 Tangent{};
        glm::vec3 BitTangent{};

        bool operator == (const MeshVertex& other) const {
            return Pos == other.Pos && Normal == other.Normal && UV == other.UV && Tangent == other.Tangent && BitTangent == other.BitTangent;
        }

        static VkVertexInputBindingDescription GetBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(MeshVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(MeshVertex, Pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(MeshVertex, Normal);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(MeshVertex, UV);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(MeshVertex, Tangent);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(MeshVertex, BitTangent);

            return attributeDescriptions;
        }

        struct Hash {
            size_t operator () (const MeshVertex& v) const noexcept {
                size_t pos = Math::Cantor(std::hash<float>{}(v.Pos.x), Math::Cantor(std::hash<float>{}(v.Pos.y), std::hash<float>{}(v.Pos.z)));
                size_t normal = Math::Cantor(std::hash<float>{}(v.Normal.x), Math::Cantor(std::hash<float>{}(v.Normal.y), std::hash<float>{}(v.Normal.z)));
                size_t uv = Math::Cantor(std::hash<float>{}(v.UV.x), std::hash<float>{}(v.UV.y));
                return Math::Cantor(pos, Math::Cantor(normal, uv));
            }
        };
    };

    struct ColorVertex {
        glm::vec3 Pos{};
        glm::vec3 Color{};

        static VkVertexInputBindingDescription GetBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(ColorVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(ColorVertex, Pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(ColorVertex, Color);

            return attributeDescriptions;
        }
    };

    struct ColorNormalVertex {
        glm::vec3 Pos{};
        glm::vec3 Color{};
        glm::vec3 Normal{};

        static VkVertexInputBindingDescription GetBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(ColorNormalVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(ColorNormalVertex, Pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(ColorNormalVertex, Color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(ColorNormalVertex, Normal);

            return attributeDescriptions;
        }
    };
}