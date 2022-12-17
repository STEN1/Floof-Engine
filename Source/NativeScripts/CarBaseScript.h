
#ifndef FLOOF_CARBASESCRIPT_H
#define FLOOF_CARBASESCRIPT_H

#include "NativeScript.h"
#include "../Components.h"
#include "../CollisionDispatcher.h"

namespace FLOOF {
    struct CarData;

    struct Engine {
        Engine() {
            velocityGraph.resize(1000, 0);
            TorqueGraph.resize(1000, 0);
        };

        float targetVelocity = 0.f;

        float breakingForce = 0.f;

        float maxEngineForce = 5000.f;  //this should be engine/velocity dependent
        float maxBreakingForce = 9000.f;
        float maxVelocity = 20.f;

        float servoTarget = 0.f;
        float steeringIncrement = SIMD_PI * 0.01f;
        float steeringClamp = SIMD_PI * 0.2f;

        float suspensionStiffness = 15000.f;
        float suspensionDamping = 8000.f;
        float suspensionCompression = 4.4f;

        float WheelFriction{2.f};

        float suspensionRestLength = 0.8;

        float TurnLowSpeed = SIMD_PI * 0.2f;
        float TurnHighSpeed = SIMD_PI * 0.1f;

        std::vector<btHinge2Constraint *> axles;

        bool DifLock{false};

        float getEngineForce(float velocity) const {
            float multiplier = (1 + (velocity / 100.f));
            return maxEngineForce * multiplier * multiplier * Gears[CurrentGear].second;
        }

        std::vector<float> velocityGraph;
        std::vector<float> TorqueGraph;
        int GraphOffset = 0;

        int CurrentGear{0};
        std::vector<std::pair<float, float>> Gears; // max velocity, max torque

    };

    class CarBaseScript : public NativeScript {
    public:
        CarBaseScript(glm::vec3 Pos);

        virtual void OnCreate(Scene *scene, entt::entity entity) override;

        virtual void OnUpdate(float deltaTime) override;

        virtual void LastUpdate(float deltaTime) override;

        void CameraUi();

        void EngineUi();

        void SetTransformData(CarData data);

        CarData GetTransformData();

        void AddToPhysicsWorld();

    protected:

        const glm::vec3 SpawnLocation;
        Engine engine;

        entt::entity frame;

        entt::entity CamTarget;
        entt::entity Camera;

        entt::entity Wheel_fr;
        entt::entity Wheel_fl;
        entt::entity Wheel_br;
        entt::entity Wheel_bl;

        entt::entity BreakLight;
        entt::entity HeadLightR;
        entt::entity HeadLightL;

        std::vector<btHinge2Constraint *> axles;
        int graphnumb = 0;

        struct CamLocation {
            CamLocation(std::string n, glm::vec3 loc, glm::vec3 target) {
                CamLoc = loc;
                CamTarget = target;
                name = n;
            }

            glm::vec3 CamLoc;
            glm::vec3 CamTarget;
            std::string name;
        };

        std::vector<CamLocation> CamLocations;

        bool BackWheelTurn{true};

        class TruckCollisionCallback : public CollisionDispatcher {
        public:
            TruckCollisionCallback(Scene *scene, entt::entity &entity) : CollisionDispatcher(scene, entity) {};

            virtual void onBeginOverlap(void *obj1, void *obj2) override;

            virtual void onOverlap(void *obj1, void *obj2) override;

            virtual void onEndOverlap(void *obj) override;
        };

        std::shared_ptr<TruckCollisionCallback> TruckCallback;

    };

}

#endif //FLOOF_CARBASESCRIPT_H
