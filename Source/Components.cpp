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
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "Application.h"

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

        RigidBody->setUserPointer(nullptr);
        RigidBody->setCollisionFlags(RigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        GhostObject = std::make_shared<btGhostObject>();
        GhostObject->setCollisionShape(CollisionShape.get());
        GhostObject->setCollisionFlags(GhostObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        GhostObject->setUserPointer(nullptr);
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

    RigidBodyComponent::RigidBodyComponent(glm::vec3 location, glm::vec3 scale, glm::vec3 rotation, const float mass,
                                           const std::string convexShape)
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

    void RigidBodyComponent::setCollisionDispatcher(void *ptr) {
        RigidBody->setUserPointer(ptr);
        CollisionShape->setUserPointer(ptr);

    }

    SoftBodyComponent::SoftBodyComponent(const float stiffness, const float conservation, const float mass,
                                         btSoftBody *body) {

        SoftBody = body;

        SoftBody->generateBendingConstraints(16);

        SoftBody->m_cfg.kVC = conservation; //Konservation coefficient
        //SoftBody->m_cfg.kPR = 0.1;
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
        meshData.setMesh(landscape->getMeshData());
        std::string path = "Assets/Terrain";
        
        meshData.MeshMaterial1.Diffuse = Texture(path + "/SnowRocky/diffuse.png", true);
        meshData.MeshMaterial1.Roughness = Texture(path + "/SnowRocky/roughness.png", true);
        meshData.MeshMaterial1.Normals = Texture(path + "/SnowRocky/normal.png", true);
        meshData.MeshMaterial1.UpdateDescriptorSet();
        
        meshData.MeshMaterial2.Diffuse = Texture(path + "/Stone/diffuse.png", true);
        meshData.MeshMaterial2.Roughness = Texture(path + "/Stone/roughness.png", true);
        meshData.MeshMaterial2.Normals = Texture(path + "/Stone/normal.png", true);
        meshData.MeshMaterial2.UpdateDescriptorSet();
        
        meshData.MeshMaterial3.Diffuse = Texture(path + "/GrassTex/diffuse.png", true);
        meshData.MeshMaterial3.Roughness = Texture(path + "/GrassTex/roughness.png", true);
        meshData.MeshMaterial3.Normals = Texture(path + "/GrassTex/normal.png", true);
        meshData.MeshMaterial3.UpdateDescriptorSet();

        //meshData.BlendTex = Texture(true);
    }

    LandscapeComponent::~LandscapeComponent() {
        auto *renderer = VulkanRenderer::Get();
        renderer->DestroyVulkanBuffer(&meshData.VertexBuffer);
        renderer->DestroyVulkanBuffer(&meshData.IndexBuffer);

        delete HeightFieldShape;
        delete landscape;
    }

    TriggerVolumeComponent::TriggerVolumeComponent(glm::vec3 location, glm::vec3 scale, glm::vec3 rotation,
                                                   const float mass, bt::CollisionPrimitive shape) {
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

        }
        Transform.setIdentity();
        auto rot = Utils::glmTobt(glm::vec3(rotation));
        btQuaternion btquat;
        btquat.setEulerZYX(rot.z(), rot.y(), rot.x());
        Transform.setRotation(btquat);
        Transform.setOrigin(btVector3(location.x, location.y, location.z));
        DefaultMotionState = std::make_shared<btDefaultMotionState>(Transform);

        btVector3 localInertia(0, 0, 0);
        if (mass != 0.f)
            CollisionShape->calculateLocalInertia(mass, localInertia);

        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, DefaultMotionState.get(), CollisionShape.get(), localInertia);
        RigidBody = std::make_shared<btRigidBody>(rbInfo);
        RigidBody->setFriction(0.5f);
        RigidBody->setRollingFriction(0.3f);
        RigidBody->setSpinningFriction(0.3f);

        RigidBody->setUserPointer(nullptr);
        RigidBody->setCollisionFlags(RigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK | btCollisionObject::CF_NO_CONTACT_RESPONSE);

    }

    void TriggerVolumeComponent::transform(const glm::vec3 location, const glm::vec3 rotation, const glm::vec3 scale) {
        btTransform trans;
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
    }

    void TriggerVolumeComponent::setCollisionDispatcher(void *ptr) {
        RigidBody->setUserPointer(ptr);
        CollisionShape->setUserPointer(ptr);
    }
}