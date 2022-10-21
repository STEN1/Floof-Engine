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

        void clear();

        void AddRigidBody(btRigidBody* body);

    private:
        entt::registry& mScene;
        std::shared_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration{nullptr};
        std::shared_ptr<btCollisionDispatcher> mDispatcher{nullptr};
        std::shared_ptr<btBroadphaseInterface> mOverlappingPairCache{nullptr};
        std::shared_ptr<btSequentialImpulseConstraintSolver> mSolver{nullptr};
        std::shared_ptr<btDiscreteDynamicsWorld> mDynamicsWorld{nullptr};


    };

    class  PhysicsDebugDraw: public btIDebugDraw{
    public:
        PhysicsDebugDraw();
        ~PhysicsDebugDraw();

        //todo add debugDrawline
        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;
        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor) override;

    private:

    };
}
#endif //FLOOF_PHYSICSSYSTEM_H
