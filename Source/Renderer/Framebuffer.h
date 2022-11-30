#pragma once
#include "VulkanRenderer.h"

namespace FLOOF {
	class Framebuffer {
	public:
		Framebuffer(uint32_t width, uint32_t height, VkFormat format);
		Framebuffer(uint32_t width, uint32_t height, VkImageView imageView, VkFormat format);
		~Framebuffer();

		void Resize(uint32_t width, uint32_t height);
		VkRenderPass GetRenderPass() { return m_RenderPass; }
		VkFramebuffer GetFramebuffer() { return m_Framebuffer; }
		VulkanTexture GetTexture() { return m_Texture; }
	private:
		void Create();
		void CreateFramebufferTexture();
		void CreateFramebufferObject(VkImageView imageView);

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
		VkFramebuffer m_ImageView = VK_NULL_HANDLE;
	};
	class DepthFramebuffer {
	public:
		DepthFramebuffer(uint32_t width, uint32_t height);
		~DepthFramebuffer();

		VkRenderPass GetRenderPass() { return m_RenderPass; };
		VkFramebuffer GetFramebuffer() { return m_Framebuffer; }
		VulkanTexture GetTexture() { return m_Texture; }
		VkExtent2D GetExtent() { return m_Extent; }

	private:
		void CreateFramebufferTexture();
		void CreateFramebufferObject();
		void CreateRenderPass();

		void DestroyFramebufferTexture();
		void DestroyFramebufferObject();
		void DestroyRenderPass();

		VulkanTexture m_Texture{};
		VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		VkFormat m_Format{};
		VkExtent2D m_Extent{};
	};
}