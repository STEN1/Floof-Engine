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
        void AddSoftBody(btSoftBody* body);
        void AddConstraint(btTypedConstraint* constraint, bool disableCollisionBetweenLinked);
        void AddVehicle(btActionInterface* vehicle);
        void AddCollisonShape(btCollisionObject* obj);

        btSoftRigidDynamicsWorld* GetWorld(){return mDynamicsWorld;}

        btSoftBodyWorldInfo* getSoftBodyWorldInfo(){return &mSoftBodyWorldInfo;}
        btDefaultVehicleRaycaster* getVehicleRayCaster(){return mVehicleRayCaster;}
    private:
        entt::registry& mScene;
        //btDefaultCollisionConfiguration* mCollisionConfiguration{nullptr};
        btSoftBodyRigidBodyCollisionConfiguration* mCollisionConfiguration{nullptr};
        btCollisionDispatcher* mDispatcher{nullptr};
        btBroadphaseInterface* mBroadPhase{nullptr};
        btSequentialImpulseConstraintSolver* mSolver{nullptr};
        btSoftRigidDynamicsWorld* mDynamicsWorld{nullptr};
        btSoftBodySolver* mSoftbodySolver{nullptr};
        btSoftBodyWorldInfo mSoftBodyWorldInfo;

        btCollisionAlgorithmCreateFunc* mBoxBoxCF{nullptr};
        btAlignedObjectArray<btSoftRigidCollisionAlgorithm*> mSoftRigidCollisionAlgorithms;

        btDefaultVehicleRaycaster* mVehicleRayCaster{nullptr};

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
