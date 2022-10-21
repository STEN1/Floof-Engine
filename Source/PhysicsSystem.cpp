//
// Created by Adrian Drevland on 21/10/2022.
//

#include "PhysicsSystem.h"
#include "Components.h"

namespace FLOOF {
    PhysicsSystem::PhysicsSystem() {

        mCollisionConfiguration = std::make_shared<btDefaultCollisionConfiguration>();
        mDispatcher = std::make_shared<btCollisionDispatcher>(mCollisionConfiguration.get());
        mOverlappingPairCache = std::make_shared<btDbvtBroadphase>();
        mSolver = std::make_shared<btSequentialImpulseConstraintSolver>();
        mDynamicsWorld = std::make_shared<btDiscreteDynamicsWorld>(mDispatcher.get(), mOverlappingPairCache.get(),
                                                                   mSolver.get(), mCollisionConfiguration.get());

        mDynamicsWorld->setGravity(btVector3(0, -9.81, 0));
    }

    PhysicsSystem::~PhysicsSystem() {

    }

    void PhysicsSystem::OnUpdate(float deltaTime) {

        if(!mScene){
            std::cout << "SCENE NOT SET IN PHYSICSYSTEM";
            return;
        }
        auto view = mScene->GetCulledScene().view<BallComponent, CollisionComponent, TransformComponent>();
        for(auto [entity, ball, collision, transform] : view.each()){
            mDynamicsWorld->addRigidBody(collision.RigidBody.get());
            mDynamicsWorld->stepSimulation(deltaTime, 10);
            //print positions of all objects
                btRigidBody *body = collision.RigidBody.get();
                if (body && body->getMotionState()) {
                    body->getMotionState()->getWorldTransform(collision.Transform);

                } else {
                    collision.Transform = collision.RigidBody->getWorldTransform();
                }

                auto loc = collision.Transform.getOrigin();
                //transform.Position = glm::vec3(loc.x(),loc.y(),loc.z());

        }

    }

    void PhysicsSystem::SetScene(Scene &scene) {
        mScene = &scene;

        auto view = scene.GetCulledScene().view<BallComponent, CollisionComponent, TransformComponent>();
        for(auto [entity, ball, collision, transform] : view.each()){
            mDynamicsWorld->addRigidBody(collision.RigidBody.get());
        }

    }
}