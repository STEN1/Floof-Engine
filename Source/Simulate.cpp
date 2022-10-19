//
// Created by Adrian Drevland on 26/09/2022.
//

#include "Simulate.h"
#include "Timer.h"

void FLOOF::Simulate::CalculateCollision(CollisionObject* obj1, CollisionObject* obj2) {
    auto& collidingTransform1 = obj1->Transform;
    auto& collidingVelocity1 = obj1->Velocity;
    auto& collidingBall1 = obj1->Ball;

    auto& collidingTransform2 = obj2->Transform;
    auto& collidingVelocity2 = obj2->Velocity;
    auto& collidingBall2 = obj2->Ball;

    auto contactNormal = Physics::GetContactNormal(collidingTransform1.Position, collidingTransform2.Position);

    auto combinedMass = collidingBall2.Mass + collidingBall1.Mass;
    auto elasticity = collidingBall2.Elasticity * collidingBall1.Elasticity;
    auto relVelocity = collidingVelocity2.Velocity - collidingVelocity1.Velocity;

    float angularComponent = glm::dot(relVelocity, contactNormal);
    float j = -(1.f + elasticity) * angularComponent / (1.f / combinedMass);
    if (angularComponent >= 0.f) { // moves opposite dirrections;
        j = 0.f;
    }
    const glm::vec3 vecImpulse = j * contactNormal;
    collidingVelocity2.Velocity += vecImpulse / combinedMass;
}

void FLOOF::Simulate::BallBallOverlap(FLOOF::CollisionObject* obj1, FLOOF::CollisionObject* obj2) {
    auto& collidingTransform1 = obj1->Transform;
    auto& collidingBall1 = obj1->Ball;
    auto& collidingTransform2 = obj2->Transform;
    auto& collidingBall2 = obj2->Ball;
    auto contactNormal = Physics::GetContactNormal(collidingTransform1.Position, collidingTransform2.Position);

    float dist = glm::length(collidingTransform1.Position - collidingTransform2.Position);
    collidingTransform2.Position += contactNormal * ((collidingBall1.Radius + collidingBall2.Radius) - dist);
    collidingBall2.CollisionSphere.pos = collidingTransform2.Position;

}

void FLOOF::Simulate::CalculateCollision(FLOOF::CollisionObject* obj, FLOOF::Triangle& triangle, TimeComponent& time, glm::vec3& friction) {
    glm::vec3 acc(Math::GravitationalPull);

    auto& transform = obj->Transform;
    auto& velocity = obj->Velocity;
    auto& ball = obj->Ball;

    glm::vec3 norm = glm::normalize(CollisionShape::ClosestPointToPointOnTriangle(transform.Position, triangle) - transform.Position);

    float angularComponent = glm::dot(velocity.Velocity, norm);
    float j = -(1.f + ball.Elasticity) * angularComponent / (1.f / ball.Mass);
    const glm::vec3 vecImpulse = j * norm;
    velocity.Velocity += vecImpulse / ball.Mass;

    // Add friction
    if (glm::length(velocity.Velocity) > 0.f) {
        const float frictionConstant = triangle.FrictionConstant;
        friction = -glm::normalize(velocity.Velocity) * (frictionConstant * ball.Mass);
    }

    auto dist = (glm::dot(transform.Position - triangle.A, triangle.N));
    transform.Position += glm::normalize(triangle.N) * (-dist + ball.Radius);
    ball.CollisionSphere.pos = transform.Position;

}
