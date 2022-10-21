#ifndef FLOOF_PHYSICSSYSTEM_H
#define FLOOF_PHYSICSSYSTEM_H

#include "btBulletDynamicsCommon.h"
#include "entt/entt.hpp"

namespace FLOOF {
    class PhysicsSystem {
    public:
        PhysicsSystem(entt::registry& scene);

        ~PhysicsSystem();

        void OnUpdate(float DeltaTime);

        void UpdateDynamicWorld();

    private:
        entt::registry& mScene;
        std::shared_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration{nullptr};
        std::shared_ptr<btCollisionDispatcher> mDispatcher{nullptr};
        std::shared_ptr<btBroadphaseInterface> mOverlappingPairCache{nullptr};
        std::shared_ptr<btSequentialImpulseConstraintSolver> mSolver{nullptr};
        std::shared_ptr<btDiscreteDynamicsWorld> mDynamicsWorld{nullptr};


    };

}
#endif //FLOOF_PHYSICSSYSTEM_H
