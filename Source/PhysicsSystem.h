#ifndef FLOOF_PHYSICSSYSTEM_H
#define FLOOF_PHYSICSSYSTEM_H

#include "btBulletDynamicsCommon.h"
#include "entt/entt.hpp"
#include "Renderer/Vertex.h"
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftRigidCollisionAlgorithm.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"

namespace FLOOF {
    class PhysicsSystem {
    public:
        PhysicsSystem(entt::registry& scene);

        ~PhysicsSystem();

        void OnUpdate(float DeltaTime);

        void UpdateDynamicWorld();

        void clear();

        void AddRigidBody(btRigidBody* body);

        void AddDebugFloor();
        void AddDebugShapes();

        btDiscreteDynamicsWorld* GetWorld(){return mDynamicsWorld;}

        btSoftBodyWorldInfo* getSoftBodyWorldInfo(){return &mSoftBodyWorldInfo;}
    private:
        entt::registry& mScene;
        btDefaultCollisionConfiguration* mCollisionConfiguration{nullptr};
        //btSoftBodyRigidBodyCollisionConfiguration* mCollisionConfiguration{nullptr}; // crashing
        btCollisionDispatcher* mDispatcher{nullptr};
        btBroadphaseInterface* mBroadPhase{nullptr};
        btSequentialImpulseConstraintSolver* mSolver{nullptr};
        //btConstraintSolver* mSolver{nullptr};
        btSoftRigidDynamicsWorld* mSoftDynamicsWorld{nullptr};
        btDiscreteDynamicsWorld* mDynamicsWorld{nullptr};
        btSoftBodyWorldInfo mSoftBodyWorldInfo;

        btCollisionAlgorithmCreateFunc* mBoxBoxCF{nullptr};
        btAlignedObjectArray<btSoftRigidCollisionAlgorithm*> mSoftRigidCollisionAlgorithms;
    };

    struct LineMeshComponent;
    class  PhysicsDebugDraw: public btIDebugDraw{
    public:
        PhysicsDebugDraw();
        ~PhysicsDebugDraw();

        LineMeshComponent* GetUpdatedLineMesh();

        void ClearDebugLines();

        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;
        virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
        virtual void reportErrorWarning(const char* warningString) override;
        virtual void draw3dText(const btVector3& location, const char* textString) override;
        virtual void setDebugMode(int debugMode) override;
        virtual int getDebugMode() const override;

    private:
        LineMeshComponent* m_LineMesh = nullptr;
        std::vector<ColorVertex> m_VertexData;
        int mDebugMode{btIDebugDraw::DBG_NoDebug};
    };
}
#endif //FLOOF_PHYSICSSYSTEM_H
