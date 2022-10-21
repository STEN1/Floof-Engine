
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
            btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(100.), btScalar(20.), btScalar(100.)));


            btTransform groundTransform;
            groundTransform.setIdentity();
            groundTransform.setOrigin(btVector3(0, -80, 0));

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
           transform.Position = glm::vec3(trans.getOrigin().getX(),trans.getOrigin().getY(),trans.getOrigin().getX());
            //printf("world pos object %d = %f,%f,%f\n", float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
        }


    }

    void PhysicsSystem::UpdateDynamicWorld() {
        auto view = mScene.view<RigidBodyComponent>();
        for(auto [entity, body]: view.each()){
            mDynamicsWorld->addRigidBody(body.RigidBody.get());
        }

    }


}