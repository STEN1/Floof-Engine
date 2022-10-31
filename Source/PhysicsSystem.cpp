
#include "PhysicsSystem.h"
#include "Components.h"
#include "BulletSoftBody/btDefaultSoftBodySolver.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

#define GravitationalConstant -9.81

namespace FLOOF {
    PhysicsSystem::PhysicsSystem(entt::registry& scene): mScene(scene) {

        mCollisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();

        mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
        mSoftBodyWorldInfo.m_dispatcher = mDispatcher;

        const int maxProxies = 32766; // defined in documentation
        const int AabbSize = 1000;
        btVector3 worldAabbMin(-AabbSize, -AabbSize, -AabbSize);
        btVector3 worldAabbMax(AabbSize, AabbSize, AabbSize);
        mBroadPhase = new btAxisSweep3(worldAabbMin, worldAabbMax, maxProxies);

        mSoftBodyWorldInfo.m_broadphase = mBroadPhase;

        mSolver = new btSequentialImpulseConstraintSolver();
        auto* softbodySolver = new  btDefaultSoftBodySolver();

        mDynamicsWorld = new btSoftRigidDynamicsWorld(mDispatcher, mBroadPhase, mSolver, mCollisionConfiguration,softbodySolver);

        mDynamicsWorld->setGravity(btVector3(0, GravitationalConstant, 0));


        mSoftBodyWorldInfo.m_gravity = mDynamicsWorld->getGravity();
        mSoftBodyWorldInfo.m_sparsesdf.Initialize();
        mDynamicsWorld->getDispatchInfo().m_enableSPU = true;
        mSoftBodyWorldInfo.air_density = (btScalar)1.2;
        mSoftBodyWorldInfo.water_density = 0;
        mSoftBodyWorldInfo.water_offset = 0;
        mSoftBodyWorldInfo.water_normal = btVector3(0, 0, 0);
        mSoftBodyWorldInfo.m_gravity.setValue(0,GravitationalConstant , 0);

    }

    PhysicsSystem::~PhysicsSystem() {
        delete mDynamicsWorld;
        delete mSolver;
        delete mBroadPhase;
        delete mDispatcher;
        delete mCollisionConfiguration;
    }

    void PhysicsSystem::OnUpdate(float deltaTime) {

        if(!mDynamicsWorld)
            return;

        mDynamicsWorld->stepSimulation(deltaTime);

        //rigid body
        {
            auto view = mScene.view<RigidBodyComponent, TransformComponent>();
            for (auto [entity, RigidBodyComponent, transform]: view.each()) {


                btRigidBody *body = RigidBodyComponent.RigidBody.get();
                btTransform trans;
                if (body && body->getMotionState()) {
                    body->getMotionState()->getWorldTransform(trans);
                } else {
                    trans = body->getWorldTransform();
                }

                float x, y, z;
                trans.getRotation().getEulerZYX(z, y, x);
                transform.Rotation = glm::vec3(x, y, z);

                transform.Position =  glm::vec3(trans.getOrigin().getX(), trans.getOrigin().getY(),trans.getOrigin().getZ());
            }
        }
        //soft body
        {
            auto view = mScene.view<SoftBodyComponent, TransformComponent>();
            for (auto [entity, SoftBodyComponent, transform]: view.each()) {

                btSoftBody *body = SoftBodyComponent.SoftBody;
                btTransform trans;

                if(!body)
                    continue;

                //draw soft body with debugger
                btSoftBodyHelpers::Draw(body,mDynamicsWorld->getDebugDrawer(),mDynamicsWorld->getDrawFlags());
                trans = body->getRigidTransform();

                transform.Position = glm::vec3(trans.getOrigin().getX(), trans.getOrigin().getY(),trans.getOrigin().getZ());
                float x, y, z;
                trans.getRotation().getEulerZYX(z, y, x);
                transform.Rotation = glm::vec3(x, y, z);
            }
        }

        mDynamicsWorld->debugDrawWorld();

    }

    void PhysicsSystem::UpdateDynamicWorld() {
        {
            auto view = mScene.view<RigidBodyComponent>();
            for (auto [entity, body]: view.each()) {
                AddRigidBody(body.RigidBody.get());
            }
        }
        {
            auto view = mScene.view<SoftBodyComponent>();
            for (auto [entity, body]: view.each()) {
                AddSoftBody(body.SoftBody);
            }
        }
    }

    void PhysicsSystem::clear() {

        if(mDynamicsWorld)
        for (int i = mDynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
        {
            btCollisionObject* obj = mDynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState())
            {
                delete body->getMotionState();
            }
            mDynamicsWorld->removeCollisionObject(obj);
        }
    }

    void PhysicsSystem::AddRigidBody(btRigidBody *body) {
        if(mDynamicsWorld)
            mDynamicsWorld->addRigidBody(body);
    }

    void PhysicsSystem::AddSoftBody(btSoftBody *body) {
        if(mDynamicsWorld)
            mDynamicsWorld->addSoftBody(body);
    }


    void PhysicsDebugDraw::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
        //btIDebugDraw::drawLine(from,to,color);
        ColorVertex vFrom;
        vFrom.Color = glm::vec3(color.x(), color.y(), color.z());
        vFrom.Pos = glm::vec3(from.x(), from.y(), from.z());

        ColorVertex vTo;
        vTo.Color = glm::vec3(color.x(), color.y(), color.z());
        vTo.Pos = glm::vec3(to.x(), to.y(), to.z());

        m_VertexData.push_back(vFrom);
        m_VertexData.push_back(vTo);
    }

    void PhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {
        ColorVertex vFrom;
        vFrom.Color = glm::vec3(color.x(), color.y(), color.z());
        vFrom.Pos = glm::vec3(PointOnB.x(), PointOnB.y(), PointOnB.z());

        ColorVertex vTo;
        vTo.Color = glm::vec3(color.x(), color.y(), color.z());
        glm::vec3 endLine = glm::vec3(vFrom.Pos + (glm::vec3(normalOnB.x(),normalOnB.y(),normalOnB.z())*distance));
        vTo.Pos = glm::vec3(endLine);

        m_VertexData.push_back(vFrom);
        m_VertexData.push_back(vTo);
    }

    void PhysicsDebugDraw::reportErrorWarning(const char* warningString) {
    }

    void PhysicsDebugDraw::draw3dText(const btVector3& location, const char* textString) {
    }

    void PhysicsDebugDraw::setDebugMode(int debugMode) {
        mDebugMode = debugMode;
    }

    int PhysicsDebugDraw::getDebugMode() const {
        return mDebugMode;
    }

    PhysicsDebugDraw::PhysicsDebugDraw() {
        m_VertexData.resize(1000000);
        memset(m_VertexData.data(), 0, m_VertexData.size() * sizeof(ColorVertex));
        m_LineMesh = new LineMeshComponent(m_VertexData);
        m_VertexData.clear();
        GetUpdatedLineMesh(); // Clears line buffer.
    }

    PhysicsDebugDraw::~PhysicsDebugDraw() {
        if (m_LineMesh)
            delete m_LineMesh;
    }
    LineMeshComponent* PhysicsDebugDraw::GetUpdatedLineMesh() {
        m_LineMesh->UpdateBuffer(m_VertexData);
        m_VertexData.clear();
        return m_LineMesh;
    }
    void PhysicsDebugDraw::ClearDebugLines() {
        m_VertexData.clear();
    }
}