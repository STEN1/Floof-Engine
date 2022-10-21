//
// Created by Adrian Drevland on 21/10/2022.
//

#include "PhysicsSystem.h"
#include "Components.h"

namespace FLOOF {
    PhysicsSystem::PhysicsSystem(Scene* scene): mScene(scene) {

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

        mDynamicsWorld->stepSimulation(deltaTime, 10);

        //print positions of all objects
        for (int j = mDynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--)
        {
            btCollisionObject* obj = mDynamicsWorld->getCollisionObjectArray()[j];
            btRigidBody* body = btRigidBody::upcast(obj);
            btTransform trans;
            if (body && body->getMotionState())
            {
                body->getMotionState()->getWorldTransform(trans);
            }
            else
            {
                trans = obj->getWorldTransform();
            }
            printf("world pos object %d = %f,%f,%f\n", j, float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));
        }


    }
}