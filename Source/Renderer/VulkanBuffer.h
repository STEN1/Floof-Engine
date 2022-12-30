#pragma once

#include "VulkanRenderer.h"
#include <vector>

namespace FLOOF {
    template<typename T>
    class VulkanUBO {
    public:
        VulkanUBO();
        VulkanUBO(const T& data, RenderSetLayouts setLayout);
        ~VulkanUBO();
        void Update(const T& data);
        VkDescriptorSet GetDescriptorSet();
    private:
        void MakeMappedBuffer();
        VulkanBufferData m_Data{};
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
        RenderSetLayouts m_SetLayout;
    };
    template<typename T>
    inline VulkanUBO<T>::VulkanUBO() {
        // TODO: REMOVE DEFAULT CONSTRUCTOR. THIS IS BAD!
        m_SetLayout = RenderSetLayouts::SceneFrameUBO;
        MakeMappedBuffer();
    }
    template<typename T>
    inline VulkanUBO<T>::VulkanUBO(const T& data, RenderSetLayouts setLayout) {
        m_SetLayout = setLayout;
        MakeMappedBuffer();
        Update(data);
    }
    template<typename T>
    inline VulkanUBO<T>::~VulkanUBO() {
        auto* renderer = VulkanRenderer::Get();
        renderer->DestroyVulkanBuffer(&m_Data);
        renderer->FreeUBODescriptorSet(m_DescriptorSet);
    }
    template<typename T>
    inline void VulkanUBO<T>::Update(const T& data) {
        memcpy(m_Data.AllocationInfo.pMappedData, &data, sizeof(T));
    }
    template<typename T>
    inline VkDescriptorSet VulkanUBO<T>::GetDescriptorSet() {
        return m_DescriptorSet;
    }
    template<typename T>
    inline void VulkanUBO<T>::MakeMappedBuffer() {
        auto* renderer = VulkanRenderer::Get();

        VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufCreateInfo.size = sizeof(T);
        bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        vmaCreateBuffer(renderer->GetAllocator(), &bufCreateInfo, &allocCreateInfo, &m_Data.Buffer, &m_Data.Allocation, &m_Data.AllocationInfo);

        if (m_DescriptorSet != VK_NULL_HANDLE)
            renderer->FreeUBODescriptorSet(m_DescriptorSet);

        m_DescriptorSet = renderer->AllocateUBODescriptorSet(renderer->GetDescriptorSetLayout(m_SetLayout));

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_Data.Buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = m_DescriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeDescriptorSet, 0, nullptr);
    }

    template<typename T>
    class VulkanSSBO {
    public:
        VulkanSSBO();
        VulkanSSBO(const std::vector<T>& data, RenderSetLayouts setLayout);
        ~VulkanSSBO();
        void Update(const std::vector<T>& data);
        VkDescriptorSet GetDescriptorSet();
    private:
        void Resize(uint32_t newSize);
        void MakeMappedBuffer();
        VulkanBufferData m_Data{};
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
        RenderSetLayouts m_SetLayout;
        uint32_t m_Size{};
    };
    template<typename T>
    inline VulkanSSBO<T>::VulkanSSBO() {
        // TODO: REMOVE DEFAULT CONSTRUCTOR. THIS IS BAD!
        m_SetLayout = RenderSetLayouts::LightSSBO;
        m_Size = sizeof(T);
        MakeMappedBuffer();
    }
    template<typename T>
    inline VulkanSSBO<T>::VulkanSSBO(const std::vector<T>& data, RenderSetLayouts setLayout) {
        m_SetLayout = setLayout;
        m_Size = data.size() * sizeof(T);
        MakeMappedBuffer();
        Update(data);
    }
    template<typename T>
    inline VulkanSSBO<T>::~VulkanSSBO() {
        auto* renderer = VulkanRenderer::Get();
        renderer->DestroyVulkanBuffer(&m_Data);
        renderer->FreeShaderStorageDescriptorSet(m_DescriptorSet);
    }
    template<typename T>
    inline void VulkanSSBO<T>::Update(const std::vector<T>& data) {
        if (data.size() * sizeof(T) > m_Size)
            Resize(data.size() * sizeof(T));
        memcpy(m_Data.AllocationInfo.pMappedData, data.data(), sizeof(T) * data.size());
    }
    template<typename T>
    inline VkDescriptorSet VulkanSSBO<T>::GetDescriptorSet() {
        return m_DescriptorSet;
    }
    template<typename T>
    inline void VulkanSSBO<T>::Resize(uint32_t newSize) {
        auto* renderer = VulkanRenderer::Get();
        renderer->DestroyVulkanBuffer(&m_Data);
        m_Size = newSize;
        MakeMappedBuffer();
    }
    template<typename T>
    inline void VulkanSSBO<T>::MakeMappedBuffer() {
        auto* renderer = VulkanRenderer::Get();

        VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufCreateInfo.size = m_Size;
        bufCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        vmaCreateBuffer(renderer->GetAllocator(), &bufCreateInfo, &allocCreateInfo, &m_Data.Buffer, &m_Data.Allocation, &m_Data.AllocationInfo);

        if (m_DescriptorSet != VK_NULL_HANDLE)
            renderer->FreeShaderStorageDescriptorSet(m_DescriptorSet);

        m_DescriptorSet = renderer->AllocateShaderStorageDescriptorSet(renderer->GetDescriptorSetLayout(m_SetLayout));

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_Data.Buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = m_Size;

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = m_DescriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSet.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeDescriptorSet, 0, nullptr);
    }
}