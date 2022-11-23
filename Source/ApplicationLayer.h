#pragma once
#include "Renderer/VulkanRenderer.h"

namespace FLOOF {
	class ApplicationLayer {
	public:
		virtual ~ApplicationLayer() {};
		virtual void OnUpdate(double deltaTime) {};
		virtual void OnImGuiUpdate(double deltaTime) {};
		virtual VkSemaphore OnDraw(double deltaTime, VkSemaphore waitSemaphore) { return VK_NULL_HANDLE; };
	};
}