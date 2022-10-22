
#include "PhysicsSystem.h"
#include "Components.h"

namespace FLOOF {
    PhysicsSystem::PhysicsSystem(entt::registry& scene): mScene(scene) {

        mCollisionConfiguration = std::make_shared<btDefaultCollisionConfiguration>();
        mDispatcher = std::make_shared<btCollisionDispatcher>(mCollisionConfiguration.get());
        mOverlappingPairCache = std::make_shared<btDbvtBroadphase>();
        mSolver = std::make_shared<btSequentialImpulseConstraintSolver>();
        mDynamicsWorld = std::make_shared<btDiscreteDynamicsWorld>(mDispatcher.get(), mOverlappingPairCache.get(),
                                                                   mSolver.get(), mCollisionConfiguration.get());

        mDynamicsWorld->setGravity(btVector3(0, -9.81, 0));

        //creating invisible floor
        {
            btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(200.), btScalar(10.), btScalar(200.)));


            btTransform groundTransform;
            groundTransform.setIdentity();
            groundTransform.setOrigin(btVector3(0, -200, 0));

            btScalar mass(0.);

            //rigidbody is dynamic if and only if mass is non zero, otherwise static
            bool isDynamic = (mass != 0.f);

            btVector3 localInertia(0, 0, 0);
            if (isDynamic)
                groundShape->calculateLocalInertia(mass, localInertia);

            //using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
            btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
            btRigidBody* body = new btRigidBody(rbInfo);

            //add the body to the dynamics world
            mDynamicsWorld->addRigidBody(body);
        }
    }

    PhysicsSystem::~PhysicsSystem() {

    }

    void PhysicsSystem::OnUpdate(float deltaTime) {

        mDynamicsWorld->stepSimulation(deltaTime, 10);

        auto view = mScene.view<RigidBodyComponent, TransformComponent>();
        for(auto [entity, RigidBodyComponent, transform]: view.each()){

            btRigidBody* body = RigidBodyComponent.RigidBody.get();
            btTransform trans;
            if (body && body->getMotionState())
            {
                body->getMotionState()->getWorldTransform(trans);
            }
            else
            {
                trans = body->getWorldTransform();
            }
            transform.Position = glm::vec3(trans.getOrigin().getX(),trans.getOrigin().getY(),trans.getOrigin().getZ());
            auto rot=trans.getRotation().getAxis();
            float x,y,z;
            trans.getRotation().getEulerZYX(z,y,x);
            transform.Rotation = glm::vec3(x,y,z);
        }

        mDynamicsWorld->debugDrawWorld();

    }

    void PhysicsSystem::UpdateDynamicWorld() {
        auto view = mScene.view<RigidBodyComponent>();
        for(auto [entity, body]: view.each()){
            mDynamicsWorld->addRigidBody(body.RigidBody.get());
        }

    }

    void PhysicsSystem::clear() {

        auto view = mScene.view<RigidBodyComponent, TransformComponent>();
        for(auto [entity, RigidBodyComponent, transform]: view.each()){
            btRigidBody* body = RigidBodyComponent.RigidBody.get();
            if (body && body->getMotionState())
            {
               delete body->getMotionState();
            }
            mDynamicsWorld->removeRigidBody(body);
            delete body;
            delete RigidBodyComponent.DefaultMotionState.get();
            delete RigidBodyComponent.CollisionShape.get();
            delete RigidBodyComponent.RigidBody.get();
        }
        //delete mDynamicsWorld.get();
        //delete mSolver.get();
        //delete mOverlappingPairCache.get();
        //delete mDispatcher.get();
        //delete mCollisionConfiguration.get();
    }

    void PhysicsSystem::AddRigidBody(btRigidBody *body) {
        mDynamicsWorld->addRigidBody(body);

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