//
// Created by Adrian Drevland on 21/10/2022.
//

#ifndef FLOOF_PHYSICSSYSTEM_H
#define FLOOF_PHYSICSSYSTEM_H

#include "btBulletDynamicsCommon.h"
#include "Scene.h"

namespace FLOOF {
    class PhysicsSystem {
    public:
        PhysicsSystem();

        ~PhysicsSystem();

        void SetScene(Scene& scene);

        void OnUpdate(float DeltaTime);

    private:
        Scene* mScene;
        std::shared_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration{nullptr};
        std::shared_ptr<btCollisionDispatcher> mDispatcher{nullptr};
        std::shared_ptr<btBroadphaseInterface> mOverlappingPairCache{nullptr};
        std::shared_ptr<btSequentialImpulseConstraintSolver> mSolver{nullptr};
        std::shared_ptr<btDiscreteDynamicsWorld> mDynamicsWorld{nullptr};


    };

}
#endif //FLOOF_PHYSICSSYSTEM_H
