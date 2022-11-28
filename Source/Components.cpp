#include "Components.h"

#include <cstring>
#include "stb_image/stb_image.h"
#include "ObjLoader.h"
#include "Utils.h"
#include "Renderer/ModelManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>

#include "SoundManager.h"

//pulls in python api
// The python headers must be included last!
// Fucks up some defines for the STL on MSVC debug builds.
#define PY_SSIZE_T_CLEAN
#ifdef WIN32
#ifdef _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#else

#include <python.h>

#endif


namespace FLOOF {
    LineMeshComponent::LineMeshComponent(const std::vector<ColorVertex> &vertexData) {
        auto renderer = VulkanRenderer::Get();

        VertexCount = vertexData.size();
        MaxVertexCount = VertexCount;
        VkBufferCreateInfo bufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
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

        VkDeviceSize offset{0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VertexBuffer.Buffer, &offset);
        vkCmdDraw(commandBuffer, VertexCount, 1, 0, 0);
    }

    void LineMeshComponent::UpdateBuffer(const std::vector<ColorVertex> &vertexData) {
        VertexCount = vertexData.size();
        if (VertexCount > MaxVertexCount) {
            VertexCount = MaxVertexCount;
        }
        // Buffer is already mapped. You can access its memory.
        memcpy(VertexBuffer.AllocationInfo.pMappedData, vertexData.data(), sizeof(ColorVertex) * VertexCount);
    }

    CameraComponent::CameraComponent()
        : CameraComponent(glm::vec3(0.f))
    {
    }

    CameraComponent::CameraComponent(glm::vec3 position) : Position{position} {
        Up = glm::vec3(0.f, -1.f, 0.f);
        Forward = glm::vec3(0.f, 0.f, 1.f);
        Right = glm::normalize(glm::cross(Forward, Up));
    }

    glm::mat4 CameraComponent::GetVP(float fov, float aspect, float near, float far) {
        glm::mat4 view = GetView();
        glm::mat4 projection = GetPerspective(fov, aspect, near, far);
        return projection * view;
    }

    glm::mat4 CameraComponent::GetView() {
        return glm::lookAt(Position, Position + Forward, Up);
    }

    glm::mat4 CameraComponent::GetPerspective(float fov, float aspect, float near, float far) {
        FOV = fov;
        Aspect = aspect;
        Near = near;
        Far = far;
        return glm::perspective(fov, aspect, near, far);
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

    void CameraComponent::Lookat(const glm::vec3 eye, const glm::vec3 center) {
        Position = eye;
        Forward = center - eye;
    }

    PointCloudComponent::PointCloudComponent(const std::vector<ColorVertex> &vertexData) {
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

        VkDeviceSize offset{0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VertexBuffer.Buffer, &offset);
        vkCmdDraw(commandBuffer, VertexCount, 1, 0, 0);
    }

    BSplineComponent::BSplineComponent(const std::vector<glm::vec3> &controllPoints) {
        Update(controllPoints);
    }

    void BSplineComponent::Update(const std::vector<glm::vec3> &controllPoints) {
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

    void BSplineComponent::AddControllPoint(const glm::vec3 &point) {
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

        RigidBody->setUserPointer(this);
        GhostObject = std::make_shared<btGhostObject>();
        GhostObject->setCollisionShape(CollisionShape.get());
        GhostObject->setCollisionFlags(GhostObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        GhostObject->setUserPointer(this);
    }

    RigidBodyComponent::RigidBodyComponent(glm::vec3 location, glm::vec3 scale, glm::vec3 rotation, const float mass,
                                           bt::CollisionPrimitive shape) : DefaultScale(scale), Primitive(shape) {

        using namespace bt;
        switch (shape) {
            case CollisionPrimitive::Box:
                CollisionShape = std::make_shared<btBoxShape>(btVector3(scale.x, scale.y, scale.z));
                break;

            case CollisionPrimitive::Sphere:
                CollisionShape = std::make_shared<btSphereShape>(scale.x);
                break;

            case CollisionPrimitive::Capsule:
                CollisionShape = std::make_shared<btCapsuleShape>(scale.x, scale.y);
                break;

            case CollisionPrimitive::Cylinder:
                CollisionShape = std::make_shared<btCylinderShape>(btVector3(scale.x, scale.y, scale.z));
                break;

            case CollisionPrimitive::Cone:
                CollisionShape = std::make_shared<btConeShape>(scale.x, scale.y * 2.f);
                break;
            case CollisionPrimitive::ConvexHull :
                assert("Pls give a convex shape file in constructor");
                auto vertices = ModelManager::LoadbtModel("Assets/LowPolySphere.fbx", scale);
                std::shared_ptr<btConvexHullShape> hullShape = std::make_shared<btConvexHullShape>(&vertices.btVertices[0].x(), vertices.VertCount, sizeof(btVector3));
                hullShape->optimizeConvexHull();
                CollisionShape = hullShape;
                break;

        }
        Transform.setIdentity();
        auto rot = Utils::glmTobt(glm::vec3(rotation));
        btQuaternion btquat;
        btquat.setEulerZYX(rot.z(), rot.y(), rot.x());
        Transform.setRotation(btquat);
        Transform.setOrigin(btVector3(location.x, location.y, location.z));
        InitializeBasicPhysics(mass);

    }

    RigidBodyComponent::RigidBodyComponent(glm::vec3 location, glm::vec3 scale, glm::vec3 rotation, const float mass,const std::string convexShape)
            : DefaultScale(scale), Primitive(bt::ConvexHull) {

        auto vertices = ModelManager::LoadbtModel(convexShape, scale);
        std::shared_ptr<btConvexHullShape> hullShape = std::make_shared<btConvexHullShape>(&vertices.btVertices[0].x(), vertices.VertCount, sizeof(btVector3));
        hullShape->optimizeConvexHull();
        CollisionShape = hullShape;

        Transform.setIdentity();
        Transform.setOrigin(btVector3(location.x, location.y, location.z));
        auto rot = Utils::glmTobt(glm::vec3(rotation));
        btQuaternion btquat;
        btquat.setEulerZYX(rot.z(), rot.y(), rot.x());
        Transform.setRotation(btquat);
        InitializeBasicPhysics(mass);

    }

    void RigidBodyComponent::transform(const glm::vec3 location, const glm::vec3 rotation, const glm::vec3 scale) {
        btTransform trans;
        //RigidBody->setActivationState(0);
        if (RigidBody && RigidBody->getMotionState()) {
            RigidBody->getMotionState()->getWorldTransform(trans);
        } else {
            trans = RigidBody->getWorldTransform();
        }
        trans.setOrigin(Utils::glmTobt(location));
        btQuaternion btquat;
        auto rot = Utils::glmTobt(rotation);
        btquat.setEulerZYX(rot.z(), rot.y(), rot.x());
        trans.setRotation(btquat);
        trans.setOrigin(Utils::glmTobt(location));
        RigidBody->setCenterOfMassTransform(trans);

        //CollisionShape->setLocalScaling(Utils::glmTobt(scale)/(Utils::glmTobt(DefaultScale)-btVector3(1.f,1.f,1.f)));

    }

    void RigidBodyComponent::wakeup() {
        RigidBody->activate(true);
    }

    SoftBodyComponent::SoftBodyComponent(const float stiffness, const float conservation, const float mass,
                                         btSoftBody *body) {

        SoftBody = body;

        SoftBody->generateBendingConstraints(16);

        SoftBody->m_cfg.kVC = conservation; //Konservation coefficient
        SoftBody->m_cfg.kPR = 0.1;
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
        SoftBody->generateClusters(2);
        SoftBody->setPose(true, true);

        SoftBody->setTotalMass(mass, false);

    }

    SoftBodyComponent::~SoftBodyComponent() {
        delete SoftBody;

    }

    ScriptComponent::ScriptComponent(const std::string PyScript) : Script(PyScript) {


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
        _putenv_s("PYTHONPATH", "Scripts");
#else
        setenv("PYTHONPATH", "Scripts", 1);
#endif

        Py_Initialize();

        ModuleName = Script.substr(8, Script.size());
        ModuleName = ModuleName.substr(0, ModuleName.size() - 3);

        Pname = PyUnicode_FromString(ModuleName.c_str());
        Pmodule = PyImport_Import(Pname);

        OnCreate();
    }

    ScriptComponent::~ScriptComponent() {

        Py_Finalize();

    }

    void ScriptComponent::RunScript() {
        auto Fp = fopen(Script.c_str(), "r");
        PyRun_SimpleFile(Fp, Script.c_str());
        fclose(Fp);

    }

    void ScriptComponent::OnCreate() {

        if (Pmodule) {
            auto pfunc = PyObject_GetAttrString(Pmodule, "create");
            if (PyCallable_Check(pfunc)) {
                auto ans = PyObject_CallObject(pfunc, NULL);
            }
        }
    }

    void ScriptComponent::OnUpdate(const float deltatime) {

        if (Pmodule) {
            auto pfunc = PyObject_GetAttrString(Pmodule, "update");
            if (PyCallable_Check(pfunc)) {
                PyObject * args = PyTuple_New(1);
                PyObject * dt = PyFloat_FromDouble(deltatime);
                PyTuple_SetItem(args, 0, dt);

                auto ans = PyObject_CallObject(pfunc, args);
            }

        }
    }

    SoundSourceComponent::SoundSourceComponent() {
    }

    SoundSourceComponent::SoundSourceComponent(const std::string& path) {
        AddClip(path);
    }


    void SoundSourceComponent::NewDeviceReload() {
        for (auto it = mClips.begin(); it != mClips.end(); it++)
            (*it).second->NewDeviceReload();
    }

    SoundSourceComponent::~SoundSourceComponent() {

    }

    void SoundSourceComponent::AddClip(const std::string& path) {
        mClips.insert(std::pair<std::string, std::shared_ptr<SoundClip>>(path, std::make_shared<SoundClip>(path)));
    }




    SoundSourceComponent::SoundClip::SoundClip(const std::string& path) {
        m_Name = path;
        m_Sound = SoundManager::LoadWav(path);
        m_Path = path;
        m_Source = SoundManager::GenerateSource(this);
        alec(alSourcei(m_Source, AL_BUFFER, m_Sound));
    }

    void SoundSourceComponent::SoundClip::NewDeviceReload() {
        // TODO Make sure previous source and buffer is deleted
        m_Sound = SoundManager::LoadWav(m_Name);
        m_Source = SoundManager::GenerateSource(this);
        alec(alSourcei(m_Source, AL_BUFFER, m_Sound));
    }

    SoundSourceComponent::SoundClip::~SoundClip() {
        SoundManager::DeleteSource(this);
    }

    void SoundSourceComponent::SoundClip::Volume(float volume) {
        m_Volume = volume;
        alec(alSourcef(m_Source, AL_GAIN, volume));
    }

    void SoundSourceComponent::SoundClip::Pitch() {
        alec(alSourcef(m_Source, AL_PITCH, m_Pitch));
    }

    void SoundSourceComponent::SoundClip::UpdateStatus() {

        ALint sourceState;
        alec(alGetSourcei(m_Source, AL_SOURCE_STATE, &sourceState))

            if (sourceState == AL_PLAYING) {
                isPlaying = true;
            }
            else {
                isPlaying = false;
            }
    }

    void SoundSourceComponent::SoundClip::Play() {
        alec(alSourcePlay(m_Source));
    }

    void SoundSourceComponent::SoundClip::Stop() {
        alec(alSourceStop(m_Source));

    }

    void SoundSourceComponent::SoundClip::Looping(bool looping) {
        if (looping) {
            alec(alSourcei(m_Source, AL_LOOPING, AL_TRUE));
            isLooping = true;
        }
        else {
            alec(alSourcei(m_Source, AL_LOOPING, AL_FALSE));
            isLooping = false;
        }
    }

    void SoundSourceComponent::SoundClip::Update() {
        Volume(m_Volume);

    }

    std::shared_ptr<SoundSourceComponent::SoundClip> SoundSourceComponent::GetClip(const std::string& name){


            if (mClips[name])
                return mClips[name];
            else
                return nullptr;
        
    }

    void ScriptComponent::ReloadScript() {
        Py_Finalize();
        Py_Initialize();
        Pname = NULL;
        Pmodule = NULL;

        ModuleName = Script.substr(8, Script.size());
        ModuleName = ModuleName.substr(0, ModuleName.size() - 3);

        Pname = PyUnicode_FromString(ModuleName.c_str());
        Pmodule = PyImport_Import(Pname);

    }

    LandscapeComponent::LandscapeComponent(const char *map, const char *texture) {
        landscape = new HeightmapLoader(map);
        landscape->readMap();

        HeightFieldShape = new HeightField(landscape->mVertices, landscape->height, landscape->width, 200.f, 0.f);
        triangleCol.height = landscape->height;
        triangleCol.width = landscape->width;

        //landscape
        {
            btVector3 aabbMin, aabbMax;
            for (int k = 0; k < 3; k++) {
                aabbMin[k] = -BT_LARGE_FLOAT;
                aabbMax[k] = BT_LARGE_FLOAT;
            }
            HeightFieldShape->mHeightfieldShape->processAllTriangles(&triangleCol, aabbMin, aabbMax);
        }

        landscape->mVertices = triangleCol.vertOut;
        landscape->mIndices = triangleCol.indicesOut;

        //vulkan data
        meshData = landscape->getMeshData();
        meshData.MeshMaterial.Diffuse = Texture(texture, true);
        meshData.MeshMaterial.UpdateDescriptorSet();
    }

    LandscapeComponent::~LandscapeComponent() {
        auto *renderer = VulkanRenderer::Get();
        renderer->DestroyVulkanBuffer(&meshData.VertexBuffer);
        renderer->DestroyVulkanBuffer(&meshData.IndexBuffer);

        delete HeightFieldShape;
        delete landscape;
    }

}