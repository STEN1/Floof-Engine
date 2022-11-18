#pragma once
#include "VulkanRenderer.h"

namespace FLOOF {
	class Framebuffer {
	public:
		Framebuffer(uint32_t width, uint32_t height, VkFormat format);
		~Framebuffer();

		void Resize(uint32_t width, uint32_t height);
	private:
		void Create();
		void CreateFramebufferTexture();
		void CreateFramebufferObject();

		void Destroy();
		void DestroyFramebufferTexture();
		void DestroyFramebufferObject();

		void CreateRenderPass();
		void DestroyRenderPass();

		VulkanTexture m_Texture{};
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
		VkFormat m_Format{};
		VkExtent2D m_Extent{};
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	};
}