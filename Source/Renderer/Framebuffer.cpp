#include "Framebuffer.h"

namespace FLOOF {
	Framebuffer::Framebuffer(uint32_t width, uint32_t height, VkFormat format)
		: m_Format(format)
	{
		m_Extent.width = width;
		m_Extent.height = height;
		CreateRenderPass();
		Create();
	}
	Framebuffer::~Framebuffer()
	{
		Destroy();
		DestroyRenderPass();
	}
	void Framebuffer::Resize(uint32_t width, uint32_t height)
	{
		m_Extent.width = width;
		m_Extent.height = height;
		Destroy();
		Create();
	}
	void Framebuffer::Create()
	{
		CreateFramebufferTexture();
		CreateFramebufferObject();
	}
	void Framebuffer::CreateFramebufferTexture()
	{
		auto* renderer = VulkanRenderer::Get();

		VkFormat format = m_Format;

		uint32_t xWidth = m_Extent.width;
		uint32_t yHeight = m_Extent.height;

		// Create Image
		VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = xWidth;
		imageInfo.extent.height = yHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;

		VmaAllocationCreateInfo imageAllocCreateInfo = {};
		imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VkResult result = vmaCreateImage(renderer->GetAllocator(), &imageInfo, &imageAllocCreateInfo, &m_Texture.Image,
			&m_Texture.Allocation, &m_Texture.AllocationInfo);
		ASSERT(result == VK_SUCCESS);

		// Create image view
		VkImageViewCreateInfo textureImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		textureImageViewInfo.image = m_Texture.Image;
		textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		textureImageViewInfo.format = format;
		textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		textureImageViewInfo.subresourceRange.baseMipLevel = 0;
		textureImageViewInfo.subresourceRange.levelCount = 1;
		textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
		textureImageViewInfo.subresourceRange.layerCount = 1;
		result = vkCreateImageView(renderer->GetDevice(), &textureImageViewInfo, nullptr, &m_Texture.ImageView);
		ASSERT(result == VK_SUCCESS);

		// Allocate and update descriptor
		VkSampler sampler = renderer->GetFontSampler();
		m_Texture.DesctriptorSet = ImGui_ImplVulkan_AddTexture(sampler, m_Texture.ImageView,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	void Framebuffer::CreateFramebufferObject()
	{
		auto* renderer = VulkanRenderer::Get();

		VkImageView attachments[] = {
					m_Texture.ImageView,
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass;
		framebufferInfo.attachmentCount = std::size(attachments);
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_Extent.width;
		framebufferInfo.height = m_Extent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(renderer->GetDevice(), &framebufferInfo, nullptr,
			&m_Framebuffer);
		ASSERT(result == VK_SUCCESS);
	}
	void Framebuffer::Destroy()
	{
		DestroyFramebufferObject();
		DestroyFramebufferTexture();
	}
	void Framebuffer::DestroyFramebufferTexture()
	{
		auto* renderer = VulkanRenderer::Get();
		vkDestroyImageView(renderer->GetDevice(), m_Texture.ImageView, nullptr);
		vmaDestroyImage(renderer->GetAllocator(), m_Texture.Image, m_Texture.Allocation);
	}
	void Framebuffer::DestroyFramebufferObject()
	{
		auto* renderer = VulkanRenderer::Get();
		vkDestroyFramebuffer(renderer->GetDevice(), m_Framebuffer, nullptr);
	}
	void Framebuffer::CreateRenderPass()
	{
		auto* renderer = VulkanRenderer::Get();

		VkAttachmentDescription colorAttachments[1]{};
		colorAttachments[0].format = m_Format;
		colorAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_NONE;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = 0;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = 0;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = std::size(colorAttachments);
		renderPassInfo.pAttachments = colorAttachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = dependencies.size();
		renderPassInfo.pDependencies = dependencies.data();

		VkResult result = vkCreateRenderPass(renderer->GetDevice(), &renderPassInfo, nullptr, &m_RenderPass);
		ASSERT(result == VK_SUCCESS);
	}
	void Framebuffer::DestroyRenderPass()
	{
		auto* renderer = VulkanRenderer::Get();
		vkDestroyRenderPass(renderer->GetDevice(), m_RenderPass, nullptr);
	}
}