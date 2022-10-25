
#include "PhysicsSystem.h"
#include "Components.h"

namespace FLOOF {
    PhysicsSystem::PhysicsSystem(entt::registry& scene): mScene(scene) {

        mCollisionConfiguration = new btDefaultCollisionConfiguration();
        mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
        mOverlappingPairCache = new btDbvtBroadphase();
        mSolver = new btSequentialImpulseConstraintSolver();
        //mSoftDynamicsWorld = new btSoftRigidDynamicsWorld(mDispatcher, mOverlappingPairCache, mSolver, mCollisionConfiguration);
        mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher, mOverlappingPairCache, mSolver, mCollisionConfiguration);
        mSoftDynamicsWorld = (btSoftRigidDynamicsWorld*)mDynamicsWorld;

        mDynamicsWorld->setGravity(btVector3(0, -9.81, 0));

    }

    PhysicsSystem::~PhysicsSystem() {
        delete mDynamicsWorld;
        delete mSolver;
        delete mOverlappingPairCache;
        delete mDispatcher;
        delete mCollisionConfiguration;
    }

    void PhysicsSystem::OnUpdate(float deltaTime) {

        if(!mDynamicsWorld)
            return;

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
            auto rot=trans.getRotation();
            float x,y,z;
            trans.getRotation().getEulerZYX(z,y,x);
            transform.Rotation = glm::vec3(x,y,z);
        }


        mDynamicsWorld->debugDrawWorld();

    }

    void PhysicsSystem::UpdateDynamicWorld() {
        auto view = mScene.view<RigidBodyComponent>();
        for(auto [entity, body]: view.each()){
            AddRigidBody(body.RigidBody.get());
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

        //delete mDynamicsWorld;
        //delete mSolver;
        //delete mOverlappingPairCache;
        //delete mDispatcher;
        //delete mCollisionConfiguration;
    }

    void PhysicsSystem::AddRigidBody(btRigidBody *body) {
        if(mDynamicsWorld)
            mDynamicsWorld->addRigidBody(body);

    }

    void PhysicsSystem::AddDebugFloor() {
        //creating invisible floor
        {
            btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(400.), btScalar(10.), btScalar(400.)));

            btTransform groundTransform;
            groundTransform.setIdentity();
            groundTransform.setOrigin(btVector3(0, -150, 0));

            btScalar mass(0.);

            btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape);
            btRigidBody* body = new btRigidBody(rbInfo);
            body->setFriction(0.5f);
            //add the body to the dynamics world
            AddRigidBody(body);
        }

    }

    void PhysicsSystem::AddDebugShapes() {
            //create random object shapes
            #define NUM_SHAPES 10
            std::vector<btCollisionShape*> m_collisionShapes;
            btCollisionShape* colShapes[NUM_SHAPES] = {
                    new btSphereShape(btScalar(5.0)),
                    new btCapsuleShape(2.5, 2.5),
                    new btCapsuleShapeX(2.5, 5.0),
                    new btCapsuleShapeZ(2.5, 5),
                    new btConeShape(2.5, 5),
                    new btConeShapeX(2.5, 5.0),
                    new btConeShapeZ(2.5, 5.0),
                    new btCylinderShape(btVector3(2.5, 5.0, 2.5)),
                    new btCylinderShapeX(btVector3(5.0, 2.5, 2.5)),
                    new btCylinderShapeZ(btVector3(2.5, 2.5, 5.0)),
            };
            for (int i = 0; i < NUM_SHAPES; i++)
                m_collisionShapes.push_back(colShapes[i]);

            /// Create Dynamic Objects
            btTransform startTransform;
            startTransform.setIdentity();

            btScalar mass(1.f);

            //rigidbody is dynamic if and only if mass is non zero, otherwise static

            float start_x = 0 - 5 / 2;
            float start_y = 0;
            float start_z = 0 - 5 / 2;

            {
                int shapeIndex = 0;
                for (int k = 0; k < 5; k++) {
                    for (int i = 0; i < 5; i++) {
                        for (int j = 0; j < 5; j++) {
                            startTransform.setOrigin( btVector3(
                                    btScalar(2.0 * i + start_x),
                                    btScalar(2.0 * j + start_z),
                                    btScalar(20 + 2.0 * k + start_y)));

                            shapeIndex++;
                            btCollisionShape *colShape = colShapes[shapeIndex % NUM_SHAPES];
                            bool isDynamic = (mass != 0.f);
                            btVector3 localInertia(0, 0, 0);

                            if (isDynamic)
                                colShape->calculateLocalInertia(mass, localInertia);

                            //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
                            btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);
                            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape,
                                                                            localInertia);
                            btRigidBody *body = new btRigidBody(rbInfo);
                            body->setFriction(1.f);
                            body->setRollingFriction(.1);
                            body->setSpinningFriction(0.1);
                            body->setAnisotropicFriction(colShape->getAnisotropicRollingFrictionDirection(),
                                                         btCollisionObject::CF_ANISOTROPIC_ROLLING_FRICTION);

                            AddRigidBody(body);
                        }
                    }
                }
            }
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