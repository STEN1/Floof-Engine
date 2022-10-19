//
// Created by Adrian Drevland on 26/09/2022.
//

#ifndef FLOOF_SIMULATE_H
#define FLOOF_SIMULATE_H

#include "Octree.h"

namespace FLOOF {
    class Simulate {
    public:
        Simulate() = delete;

        static void CalculateCollision(CollisionObject* obj1, CollisionObject* obj2);
        static void BallBallOverlap(CollisionObject* obj1, CollisionObject* obj2);
        static void CalculateCollision(CollisionObject* obj, Triangle& triangle, TimeComponent& time, glm::vec3& friction);

    };
}



#endif //FLOOF_SIMULATE_H
