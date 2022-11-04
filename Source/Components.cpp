#include "Components.h"

#include <cstring>
#include "stb_image/stb_image.h"
#include "ObjLoader.h"
#include "LoggerMacros.h"
#include "Utils.h"
#include "Renderer/ModelManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>

namespace FLOOF {
    TextureComponent::TextureComponent(const std::string& path) {
        auto it = s_TextureDataCache.find(path);
        if (it != s_TextureDataCache.end()) {
            Data = it->second;
            return;
        }

        Data.Path = path;

        auto renderer = VulkanRenderer::Get();
        // Load texture
        int xWidth, yHeight, channels;
        stbi_set_flip_vertically_on_load(false);
        auto* data = stbi_load(path.c_str(), &xWidth, &yHeight, &channels, 0);
        ASSERT(data);
        uint32_t size = xWidth * yHeight * 4;

        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

        // staging buffer
        VkBufferCreateInfo stagingCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingCreateInfo.size = size;
        stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingBufAllocCreateInfo = {};
        stagingBufAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingBufAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer{};
        VmaAllocation stagingBufferAlloc{};
        VmaAllocationInfo stagingBufferAllocInfo{};
        vmaCreateBuffer(renderer->m_Allocator, &stagingCreateInfo, &stagingBufAllocCreateInfo, &stagingBuffer,
            &stagingBufferAlloc, &stagingBufferAllocInfo);

        ASSERT(stagingBufferAllocInfo.pMappedData != nullptr);
        if (channels == 4) {
            memcpy(stagingBufferAllocInfo.pMappedData, data, size);
        } else if (channels == 3) {
            std::vector<stbi_uc> readyData(size);
            for (uint32_t h = 0; h < yHeight; h++) {
                for (uint32_t w = 0; w < xWidth; w++) {
                    uint32_t readyDataIndex = (h * xWidth * 4) + (w * 4);
                    auto& rdR = readyData[readyDataIndex];
                    auto& rdG = readyData[readyDataIndex + 1];
                    auto& rdB = readyData[readyDataIndex + 2];
                    auto& rdA = readyData[readyDataIndex + 3];

                    uint32_t dataIndex = (h * xWidth * 3) + (w * 3);
                    auto& dR = data[dataIndex];
                    auto& dG = data[dataIndex + 1];
                    auto& dB = data[dataIndex + 2];
                    auto& dA = data[dataIndex + 3];

                    rdR = dR; rdG = dG; rdB = dB;
                    rdA = (stbi_uc)255;
                }
            }
            memcpy(stagingBufferAllocInfo.pMappedData, readyData.data(), size);
        } else {
            ASSERT(false);
        }
        
        stbi_image_free(data);

        // Image
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
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->m_Allocator, &imageInfo, &imageAllocCreateInfo, &Data.CombinedTextureSampler.Image,
            &Data.CombinedTextureSampler.Allocation, &Data.CombinedTextureSampler.AllocationInfo);

        // copy image from staging buffer to image buffer(gpu only memory)
        renderer->CopyBufferToImage(stagingBuffer, Data.CombinedTextureSampler.Image, xWidth, yHeight);

        // free staging buffer
        vmaDestroyBuffer(renderer->m_Allocator, stagingBuffer, stagingBufferAlloc);

        // create image view
        VkImageViewCreateInfo textureImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        textureImageViewInfo.image = Data.CombinedTextureSampler.Image;
        textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        textureImageViewInfo.format = format;
        textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        textureImageViewInfo.subresourceRange.baseMipLevel = 0;
        textureImageViewInfo.subresourceRange.levelCount = 1;
        textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
        textureImageViewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(renderer->m_LogicalDevice, &textureImageViewInfo, nullptr, &Data.CombinedTextureSampler.ImageView);

        // sampler
        //VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        //samplerInfo.magFilter = VK_FILTER_LINEAR;
        //samplerInfo.minFilter = VK_FILTER_LINEAR;
        //samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        //samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        //samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        //samplerInfo.anisotropyEnable = VK_TRUE;
        //samplerInfo.maxAnisotropy = 16;
        //samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        //samplerInfo.unnormalizedCoordinates = VK_FALSE;
        //samplerInfo.compareEnable = VK_FALSE;
        //samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        //samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        //samplerInfo.mipLodBias = 0.f;
        //samplerInfo.minLod = 0.f;
        //samplerInfo.maxLod = FLT_MAX;

        //VkSamplerCreateInfo samplerInfo = {};
        //samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        //samplerInfo.magFilter = VK_FILTER_LINEAR;
        //samplerInfo.minFilter = VK_FILTER_LINEAR;
        //samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        //samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        //samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        //samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        //samplerInfo.minLod = -1000;
        //samplerInfo.maxLod = 1000;
        //samplerInfo.maxAnisotropy = 1.0f;
        //vkCreateSampler(renderer->m_LogicalDevice, &samplerInfo, nullptr, &Data.CombinedTextureSampler.Sampler);

        VkSampler sampler = renderer->GetSampler();

        // Get descriptor set and point it to data.
        Data.DesctriptorSet = renderer->AllocateTextureDescriptorSet();

        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.sampler = sampler;
        descriptorImageInfo.imageView = Data.CombinedTextureSampler.ImageView;
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = Data.DesctriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.pImageInfo = &descriptorImageInfo;

        vkUpdateDescriptorSets(renderer->m_LogicalDevice, 1, &writeDescriptorSet, 0, nullptr);

        s_TextureDataCache[path] = Data;
    }

    TextureComponent::~TextureComponent() {
    }

    void TextureComponent::Bind(VkCommandBuffer commandBuffer) {
        auto renderer = VulkanRenderer::Get();

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->GetPipelineLayout(RenderPipelineKeys::Basic),
            0, 1, &Data.DesctriptorSet, 0, 0);
    }

    void TextureComponent::ClearTextureDataCache() {
        auto renderer = VulkanRenderer::Get();

        for (auto& [key, data] : s_TextureDataCache) {
            renderer->FreeTextureDescriptorSet(data.DesctriptorSet);
            vkDestroyImageView(renderer->m_LogicalDevice, data.CombinedTextureSampler.ImageView, nullptr);
            vmaDestroyImage(renderer->m_Allocator, data.CombinedTextureSampler.Image, data.CombinedTextureSampler.Allocation);
            //vkDestroySampler(renderer->m_LogicalDevice, data.CombinedTextureSampler.Sampler, nullptr);
        }
    }

    MeshComponent::MeshComponent(const std::string& path) {
        auto* renderer = VulkanRenderer::Get();

        auto it = s_MeshDataCache.find(path);
        if (it == s_MeshDataCache.end()) {
            auto [vertexData, indexData] = ObjLoader(path).GetIndexedData();
            Data.VertexBuffer = renderer->CreateVertexBuffer(vertexData);
            Data.IndexBuffer = renderer->CreateIndexBuffer(indexData);
            Data.VertexCount = vertexData.size();
            Data.IndexCount = indexData.size();
            Data.Path = path;
            s_MeshDataCache[path] = Data;
        }
        else {
            Data = it->second;
        }
        m_IsCachedMesh = true;
    }

    MeshComponent::MeshComponent(const std::vector<MeshVertex>& vertexData, const std::vector<uint32_t>& indexData) {
        auto* renderer = VulkanRenderer::Get();

        Data.VertexBuffer = renderer->CreateVertexBuffer(vertexData);
        Data.IndexBuffer = renderer->CreateIndexBuffer(indexData);
        Data.VertexCount = vertexData.size();
        Data.IndexCount = indexData.size();
    }

    MeshComponent::MeshComponent(const std::vector<ColorNormalVertex>& vertexData, const std::vector<uint32_t>& indexData) {
        auto* renderer = VulkanRenderer::Get();

        Data.VertexBuffer = renderer->CreateVertexBuffer(vertexData);
        Data.IndexBuffer = renderer->CreateIndexBuffer(indexData);
        Data.VertexCount = vertexData.size();
        Data.IndexCount = indexData.size();
    }

    MeshComponent::MeshComponent(const std::vector<MeshVertex>& vertexData) {
        auto* renderer = VulkanRenderer::Get();

        Data.VertexBuffer = renderer->CreateVertexBuffer(vertexData);
        Data.VertexCount = vertexData.size();
    }

    MeshComponent::~MeshComponent() {
        if (m_IsCachedMesh == false) {
            auto* renderer = VulkanRenderer::Get();
            vmaDestroyBuffer(renderer->m_Allocator, Data.IndexBuffer.Buffer, Data.IndexBuffer.Allocation);
            vmaDestroyBuffer(renderer->m_Allocator, Data.VertexBuffer.Buffer, Data.VertexBuffer.Allocation);
        }
    }

    void MeshComponent::Draw(VkCommandBuffer commandBuffer) {
        VkDeviceSize offset{ 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &Data.VertexBuffer.Buffer, &offset);
        if (Data.IndexBuffer.Buffer != VK_NULL_HANDLE) {
            vkCmdBindIndexBuffer(commandBuffer, Data.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, Data.IndexCount,
                1, 0, 0, 0);
        }
        else {
            vkCmdDraw(commandBuffer, Data.VertexCount, 1, 0, 0);
        }
    }

    void MeshComponent::ClearMeshDataCache() {
        auto* renderer = VulkanRenderer::Get();
        for (auto& [key, data] : s_MeshDataCache) {
            vmaDestroyBuffer(renderer->m_Allocator, data.IndexBuffer.Buffer, data.IndexBuffer.Allocation);
            vmaDestroyBuffer(renderer->m_Allocator, data.VertexBuffer.Buffer, data.VertexBuffer.Allocation);
        }
    }

    LineMeshComponent::LineMeshComponent(const std::vector<ColorVertex>& vertexData) {
        auto renderer = VulkanRenderer::Get();

        VertexCount = vertexData.size();
        MaxVertexCount = VertexCount;
        VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufCreateInfo.size = sizeof(ColorVertex) * VertexCount;
        bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        vmaCreateBuffer(renderer->m_Allocator, &bufCreateInfo, &allocCreateInfo, &VertexBuffer.Buffer,
            &VertexBuffer.Allocation, &VertexBuffer.AllocationInfo);

        // Buffer is already mapped. You can access its memory.
        memcpy(VertexBuffer.AllocationInfo.pMappedData, vertexData.data(), sizeof(ColorVertex) * VertexCount);
    }

    LineMeshComponent::~LineMeshComponent() {
        auto renderer = VulkanRenderer::Get();
        if (VertexBuffer.Buffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(renderer->m_Allocator, VertexBuffer.Buffer, VertexBuffer.Allocation);
    }

    void LineMeshComponent::Draw(VkCommandBuffer commandBuffer) {
        if (VertexCount == 0)
            return;

        VkDeviceSize offset{ 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VertexBuffer.Buffer, &offset);
        vkCmdDraw(commandBuffer, VertexCount, 1, 0, 0);
    }

    void LineMeshComponent::UpdateBuffer(const std::vector<ColorVertex>& vertexData) {
        VertexCount = vertexData.size();
        if (VertexCount > MaxVertexCount) {
            VertexCount = MaxVertexCount;
        }
        // Buffer is already mapped. You can access its memory.
        memcpy(VertexBuffer.AllocationInfo.pMappedData, vertexData.data(), sizeof(ColorVertex) * VertexCount);
    }

    CameraComponent::CameraComponent(glm::vec3 position) : Position{ position } {
        Up = glm::vec3(0.f, -1.f, 0.f);
        Forward = glm::vec3(0.f, 0.f, 1.f);
        Right = glm::normalize(glm::cross(Forward, Up));
    }

    glm::mat4 CameraComponent::GetVP(float fov, float aspect, float near, float far) {
        FOV = fov;
        Aspect = aspect;
        Near = near;
        Far = far;
        glm::mat4 view = glm::lookAt(Position, Position + Forward, Up);
        glm::mat4 projection = glm::perspective(fov, aspect, near, far);
        return projection * view;
    }

    void CameraComponent::MoveForward(float amount) {
        Position += Forward * amount;
    }

    void CameraComponent::MoveRight(float amount) {
        glm::vec3 right = glm::normalize(glm::cross(Forward, Up));
        Position += right * amount;
    }

    void CameraComponent::Pitch(float amount) {
        if (amount == 0.f) return;
        glm::vec3 right = glm::normalize(glm::cross(Forward, Up));
        glm::mat4 rotation = glm::rotate(amount, right);
        Forward = glm::normalize(glm::vec3(rotation * glm::vec4(Forward, 1.f)));
    }

    void CameraComponent::Yaw(float amount) {
        if (amount == 0.f) return;
        glm::mat4 rotation = glm::rotate(-amount, Up);
        Forward = glm::normalize(glm::vec3(rotation * glm::vec4(Forward, 1.f)));
    }

    void CameraComponent::MoveUp(float amount) {
        if (amount == 0.f) return;
        Position.y += amount;

    }

    PointCloudComponent::PointCloudComponent(const std::vector<ColorVertex>& vertexData) {
        auto renderer = VulkanRenderer::Get();

        VertexBuffer = renderer->CreateVertexBuffer(vertexData);
        VertexCount = vertexData.size();
    }

    PointCloudComponent::~PointCloudComponent() {
        auto renderer = VulkanRenderer::Get();

        vmaDestroyBuffer(renderer->m_Allocator, VertexBuffer.Buffer, VertexBuffer.Allocation);
    }

    void PointCloudComponent::Draw(VkCommandBuffer commandBuffer) {
        auto renderer = VulkanRenderer::Get();

        VkDeviceSize offset{ 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VertexBuffer.Buffer, &offset);
        vkCmdDraw(commandBuffer, VertexCount, 1, 0, 0);
    }

    BSplineComponent::BSplineComponent(const std::vector<glm::vec3>& controllPoints) {
        Update(controllPoints);
    }

    void BSplineComponent::Update(const std::vector<glm::vec3>& controllPoints) {
        ASSERT(controllPoints.size() >= D + 1);
        ControllPoints = controllPoints;

        KnotPoints.resize(ControllPoints.size() + D + 1);

        int tValue = 0;
        for (uint32_t i = 0; i < (D + 1); i++) {
            KnotPoints[i] = tValue;
        }

        tValue++;

        for (uint32_t i = (D + 1); i < (KnotPoints.size() - (D + 1)); i++) {
            KnotPoints[i] = tValue++;
        }

        for (uint32_t i = (KnotPoints.size() - (D + 1)); i < KnotPoints.size(); i++) {
            KnotPoints[i] = tValue;
        }

        TMax = static_cast<float>(tValue);
    }

    void BSplineComponent::AddControllPoint(const glm::vec3& point) {
        ASSERT(ControllPoints.size() >= D + 1);
        ControllPoints.push_back(point);
        KnotPoints.push_back(KnotPoints[KnotPoints.size() - 1]);

        for (uint32_t i = (KnotPoints.size() - (D + 1)); i < KnotPoints.size(); i++) {
            KnotPoints[i]++;
        }

        TMax = KnotPoints[KnotPoints.size() - 1];
    }

    int BSplineComponent::FindKnotInterval(float t) {
        int my = ControllPoints.size() - 1;
        while (t < KnotPoints[my] && my > D) {
            my--;
        }
        return my;
    }

    glm::vec3 BSplineComponent::EvaluateBSpline(float t) {
        int my = FindKnotInterval(t);

        glm::vec3 a[D + 1];

        for (int i = 0; i <= D; i++) {
            a[D - i] = ControllPoints[my - i];
        }

        for (int k = D; k > 0; k--) {
            int j = my - k;
            for (int i = 0; i < k; i++) {
                j++;
                float w = (t - KnotPoints[j]) / (KnotPoints[j + k] - KnotPoints[j]);
                a[i] = a[i] * (1.f - w) + a[i + 1] * w;
            }
        }
        return a[0];
    }

    BSplineComponent::BSplineComponent() {

    }

    bool BSplineComponent::Isvalid() {

        return ControllPoints.size() > (D + 1);
    }

    void RigidBodyComponent::InitializeBasicPhysics(const float mass) {
        DefaultMotionState = std::make_shared<btDefaultMotionState>(Transform);

        btVector3 localInertia(0, 0, 0);
        if (mass != 0.f)
            CollisionShape->calculateLocalInertia(mass, localInertia);

        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, DefaultMotionState.get(), CollisionShape.get(), localInertia);
        RigidBody = std::make_shared<btRigidBody>(rbInfo);
        RigidBody->setFriction(0.5f);
        RigidBody->setRollingFriction(0.3f);
        RigidBody->setSpinningFriction(0.3f);
    }

    SoundComponent::SoundComponent(SoundManager* manager, std::string path) {
        id = manager->loadPath(path);
    }



    RigidBodyComponent::RigidBodyComponent(glm::vec3 location, glm::vec3 scale, const float mass,
                                           bt::CollisionPrimitive shape) : DefaultScale(scale), Primitive(shape){

        using namespace bt;
        switch (shape){
            case CollisionPrimitive::Box:
                CollisionShape = std::make_shared<btBoxShape>(btVector3(scale.x,scale.y,scale.z));
                break;

            case CollisionPrimitive::Sphere:
                CollisionShape = std::make_shared<btSphereShape>(scale.x);
                break;

            case CollisionPrimitive::Capsule:
                CollisionShape = std::make_shared<btCapsuleShape>(scale.x,scale.y);
                break;

            case CollisionPrimitive::Cylinder:
                CollisionShape = std::make_shared<btCylinderShape>(btVector3(scale.x,scale.y,scale.z));
                break;

            case CollisionPrimitive::Cone:
                CollisionShape = std::make_shared<btConeShape>(scale.x,scale.y*2.f);
                break;
            case CollisionPrimitive::ConvexHull :
                assert("Pls give a convex shape file in constructor");
                auto vertices = ModelManager::Get().LoadbtModel("Assets/LowPolySphere.fbx",scale);
                std::shared_ptr<btConvexHullShape> hullShape = std::make_shared<btConvexHullShape>(&vertices.btVertices[0].x(), vertices.VertCount, sizeof (btVector3));
                hullShape->optimizeConvexHull();
                CollisionShape = hullShape;
                break;

        }
        Transform.setIdentity();
        Transform.setOrigin(btVector3(location.x,location.y,location.z));
        InitializeBasicPhysics(mass);

    }

    RigidBodyComponent::RigidBodyComponent(glm::vec3 location, glm::vec3 scale, const float mass,const std::string convexShape)  :DefaultScale(scale), Primitive(bt::ConvexHull){

        auto vertices = ModelManager::Get().LoadbtModel(convexShape,scale);
        std::shared_ptr<btConvexHullShape> hullShape = std::make_shared<btConvexHullShape>(&vertices.btVertices[0].x(), vertices.VertCount, sizeof (btVector3));
        hullShape->optimizeConvexHull();
        CollisionShape = hullShape;

        Transform.setIdentity();
        Transform.setOrigin(btVector3(location.x,location.y,location.z));
        InitializeBasicPhysics(mass);

    }

    void RigidBodyComponent::transform(const glm::vec3 location, const glm::vec3 rotation,const glm::vec3 scale) {
        btTransform trans;
        RigidBody->setActivationState(0);
        if (RigidBody && RigidBody->getMotionState()) {
            RigidBody->getMotionState()->getWorldTransform(trans);
        } else {
            trans = RigidBody->getWorldTransform();
        }
        RigidBody->translate(Utils::glmTobt(location)-trans.getOrigin());
        trans.setOrigin(Utils::glmTobt(location));
        btQuaternion btquat;
        auto rot = Utils::glmTobt(rotation);
        btquat.setEulerZYX(rot.z(),rot.y(),rot.x());
        trans.setRotation(btquat);
        trans.setOrigin(Utils::glmTobt(location));
        RigidBody->setCenterOfMassTransform(trans);
        //RigidBody->getMotionState()->setWorldTransform(trans);

        CollisionShape->setLocalScaling(Utils::glmTobt(scale)/(Utils::glmTobt(DefaultScale)-btVector3(1.f,1.f,1.f)));

    }

    void RigidBodyComponent::wakeup() {
        RigidBody->activate(true);
    }

    SoftBodyComponent::SoftBodyComponent(const float stiffness, const float conservation,const float mass,btSoftBody* body) {

        SoftBody = body;
        SoftBody->m_cfg.kVC = conservation; //Konservation coefficient
        SoftBody->m_materials[0]->m_kLST = stiffness; // linear stiffness

        //soft rigid collision and soft soft collision
        SoftBody->m_cfg.piterations = 2;
        SoftBody->m_cfg.kDF = 1;
        SoftBody->m_cfg.kSSHR_CL = 1;
        SoftBody->m_cfg.kSS_SPLT_CL = 0;
        SoftBody->m_cfg.kSKHR_CL = 0.1f;
        SoftBody->m_cfg.kSK_SPLT_CL = 1;
        SoftBody->m_cfg.collisions = btSoftBody::fCollision::CL_SS + btSoftBody::fCollision::CL_RS;
        SoftBody->randomizeConstraints();
        SoftBody->generateClusters(16);
        SoftBody->setPose(true, false);

        SoftBody->setTotalMass(mass, true);

    }

    ScriptComponent::ScriptComponent(const std::string PyScript) :Script(PyScript){

        Py_Initialize();



    }

    ScriptComponent::~ScriptComponent() {

        Py_Finalize();

        delete Pint;
    }

    void ScriptComponent::RunScript() {
        Fp = fopen(Script.c_str(),"r");
        PyRun_SimpleFile(Fp,Script.c_str());
        fclose(Fp);

    }

    void ScriptComponent::updateScripts() {
        std::string source = SOURCE_DIR;
        source.append("/");
        source.append(Script);

        //std::string binary = BINARY_DIR;
        std::string binary = std::filesystem::current_path();
        binary.append("/");
        binary.append(Script);

         std::fstream write(binary);
         std::fstream read(source);

         std::string line;
         while(getline(read,line)){
            write << line << "\n";
         }
         write.close();
         read.close();

    }
}