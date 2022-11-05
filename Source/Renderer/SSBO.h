#pragma once
#include "VulkanRenderer.h"
#include <vector>

namespace FLOOF {
    template<typename T>
    class SSBO {
    public:
        SSBO() = delete;
        SSBO(const std::vector<T> data);
        void Update(const std::vector<T> data);
        void Resize(uint32_t newSize);
    private:
        uint32_t m_Size{};
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
        VulkanBuffer m_Buffer{};
    };

    template<typename T>
    inline SSBO<T>::SSBO(const std::vector<T> data) {

    }
    template<typename T>
    inline void SSBO<T>::Update(const std::vector<T> data) {
        if (data.size() > m_Size)
            Resize(data.size());
    }
    template<typename T>
    inline void SSBO<T>::Resize(uint32_t newSize) {

    }
}