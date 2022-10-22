#ifndef FLOOF_PHYSICSSYSTEM_H
#define FLOOF_PHYSICSSYSTEM_H

#include "btBulletDynamicsCommon.h"
#include "entt/entt.hpp"
#include "Renderer/Vertex.h"

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

    struct LineMeshComponent;
    class  PhysicsDebugDraw: public btIDebugDraw{
    public:
        PhysicsDebugDraw();
        ~PhysicsDebugDraw();

        LineMeshComponent* GetUpdatedLineMesh();

        void ClearDebugLines();

        //todo add debugDrawline
        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;
        virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
        virtual void reportErrorWarning(const char* warningString) override;
        virtual void draw3dText(const btVector3& location, const char* textString) override;
        virtual void setDebugMode(int debugMode) override;
        virtual int getDebugMode() const override;

    private:
        LineMeshComponent* m_LineMesh = nullptr;
        std::vector<ColorVertex> m_VertexData;
    };
}
#endif //FLOOF_PHYSICSSYSTEM_H
