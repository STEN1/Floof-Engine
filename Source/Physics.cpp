#include "Physics.h"

#include <cmath>
#include <algorithm>
#include "Octree.h"
#include "LoggerMacros.h"
#include "Components.h"


namespace FLOOF {
    bool Physics::PlaneBallIntersect(const Triangle& triangle, const glm::vec3& position, const float& radius) {
        auto intersect = (glm::dot(position - triangle.A, triangle.N));
        return (intersect < radius);
    }
    glm::vec3 Physics::GetReflectVelocity(const glm::vec3& velocity, const glm::vec3& reflectionAngle) {
        return velocity - (2.f * (velocity * reflectionAngle) * reflectionAngle);
    }

    glm::vec3 Physics::GetReflectionAngle(const glm::vec3& m, const glm::vec3& n) {
        return glm::normalize((m + n) / ((glm::length(m + n) * glm::length(m + n))));
    }

    glm::vec3 Physics::GetAccelerationVector(const Triangle& triangle) {
        return (float(Math::Gravity) * glm::vec3(triangle.N.x * triangle.N.y, (triangle.N.y * triangle.N.y) - 1, (triangle.N.z * triangle.N.y)));
    }

    void Physics::ElasticCollision(glm::vec3 p1, glm::vec3 p2, glm::vec3& v1, glm::vec3& v2) {
        glm::vec3 normal = (p1 - p2) / glm::length(p1 - p2);
        normal *= glm::dot(v1 - v2, normal);
        v1 -= normal;
        v2 += normal;
    }

    glm::vec3 Physics::GetContactNormal(const glm::vec3& pos1, const glm::vec3& pos2) {
        glm::vec3 contactNormal{ Math::GetSafeNormal() };
        if (glm::length(pos1 - pos2) != 0)
            contactNormal = glm::normalize(pos2 - pos1);
        return contactNormal;
    }

    /*
        void Physics::CalculateCollision(Octree::CollisionObject *obj1, Octree::CollisionObject *obj2) {

            auto &collidingTransform1 = obj1->Transform;
            auto &collidingVelocity1 = obj1->Velocity;
            auto &collidingBall1 = obj1->Ball;

            auto &collidingTransform2 = obj2->Transform;
            auto &collidingVelocity2 = obj2->Velocity;
            auto &collidingBall2 = obj2->Ball;

            auto contactNormal = Physics::GetContactNormal(collidingTransform1.Position, collidingTransform2.Position);

            auto combinedMass = collidingBall2.Mass + collidingBall1.Mass;
            auto elasticity = collidingBall2.Elasticity * collidingBall1.Elasticity;
            auto relVelocity = collidingVelocity2.Velocity - collidingVelocity1.Velocity;

            float moveangle = glm::dot(relVelocity, contactNormal);
            float j = -(1.f + elasticity) * moveangle / (1.f / combinedMass);
            if (moveangle >= 0.f) { // moves opposite dirrections;
                j = 0.f;
            }
            const glm::vec3 vecImpulse = j * contactNormal;
            collidingVelocity2.Velocity += vecImpulse / combinedMass;

        }
    */

    float CollisionShape::DistanceFromPointToPlane(const glm::vec3& point, const glm::vec3& planePos, const glm::vec3& planeNormal) {
        return glm::dot(point - planePos, planeNormal);
    }

    glm::vec3 CollisionShape::ClosestPointToPointOnTriangle(const glm::vec3& point, const Triangle& triangle) {
        // Check if P in vertex region outside A
        glm::vec3 ab = triangle.B - triangle.A;
        glm::vec3 ac = triangle.C - triangle.A;
        glm::vec3 ap = point - triangle.A;
        float d1 = glm::dot(ab, ap);
        float d2 = glm::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f)
            return triangle.A; // barycentric coordinates (1,0,0)

        // Check if P in vertex region outside B
        glm::vec3 bp = point - triangle.B;
        float d3 = glm::dot(ab, bp);
        float d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
            return triangle.B; // barycentric coordinates (0,1,0)

        // Check if P in edge region of AB, if so return projection of P onto AB
        float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            float v = d1 / (d1 - d3);
            return triangle.A + v * ab; // barycentric coordinates (1-v,v,0)
        }

        // Check if P in vertex region outside C
        glm::vec3 cp = point - triangle.C;
        float d5 = glm::dot(ab, cp);
        float d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
            return triangle.C; // barycentric coordinates (0,0,1)

        // Check if P in edge region of AC, if so return projection of P onto AC
        float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            float w = d2 / (d2 - d6);
            return triangle.A + w * ac; // barycentric coordinates (1-w,0,w)
        }

        // Check if P in edge region of BC, if so return projection of P onto BC
        float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return triangle.B + w * (triangle.C - triangle.B); // barycentric coordinates (0,1-w,w)
        }

        // P inside face region. Compute Q through its barycentric coordinates (u,v,w)
        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        return triangle.A + ab * v + ac * w; // = u*a + v*b + w*c, u = va * denom = 1.0f-v-w
    }

    bool CollisionShape::Intersect(CollisionShape* shape) {
        LOG_ERROR("Collision with none");
        return false;
    }

    bool CollisionShape::Intersect(AABB* a, AABB* b) {
        auto& aExtent = a->extent;
        auto& aPos = a->pos;

        auto& bExtent = b->extent;
        auto& bPos = b->pos;

        float xDist = abs(aPos.x - bPos.x);
        float yDist = abs(aPos.y - bPos.y);
        float zDist = abs(aPos.z - bPos.z);
        float xMinDist = aExtent.x + bExtent.x;
        float yMinDist = aExtent.y + bExtent.y;
        float zMinDist = aExtent.z + bExtent.z;

        return (xDist < xMinDist&& yDist < yMinDist&& zDist < zMinDist);
    }

    bool CollisionShape::Intersect(AABB* box, Sphere* sphere) {
        glm::vec3 boxPoint;
        boxPoint.x = std::clamp<float>(sphere->pos.x, box->pos.x - box->extent.x, box->pos.x + box->extent.x);
        boxPoint.y = std::clamp<float>(sphere->pos.y, box->pos.y - box->extent.y, box->pos.y + box->extent.y);
        boxPoint.z = std::clamp<float>(sphere->pos.z, box->pos.z - box->extent.z, box->pos.z + box->extent.z);
        float dist = glm::distance(boxPoint, sphere->pos);
        return (dist < sphere->radius);
    }

    bool CollisionShape::Intersect(Sphere* a, Sphere* b) {
        float minDist = a->radius + b->radius;
        float dist = glm::distance(a->pos, b->pos);
        return (dist < minDist);
    }

    bool CollisionShape::Intersect(AABB* aabb, Plane* plane) {
        float r =
            aabb->extent.x * glm::abs(plane->normal.x) +
            aabb->extent.y * glm::abs(plane->normal.y) +
            aabb->extent.z * glm::abs(plane->normal.z);
        return DistanceFromPointToPlane(aabb->pos, plane->pos, plane->normal) < -r;
    }

    bool CollisionShape::Intersect(Sphere* sphere, Plane* plane) {
        auto distance = DistanceFromPointToPlane(sphere->pos, plane->pos, plane->normal);
        return distance < -sphere->radius; // Ericson p.161
    }

    bool CollisionShape::Intersect(Plane* a, Plane* b) {
        return glm::abs(a->normal) != glm::abs(b->normal);
    }

    bool CollisionShape::Intersect(AABB* aabb, OBB* obb) {
        // TODO:
        return false;
    }

    bool CollisionShape::Intersect(Sphere* sphere, OBB* obb) {
        // TODO:
        return false;
    }

    bool CollisionShape::Intersect(Plane* plane, OBB* obb) {
        float r =
            obb->extent.x * glm::abs(glm::dot(plane->normal, obb->normals[0])) +
            obb->extent.y * glm::abs(glm::dot(plane->normal, obb->normals[1])) +
            obb->extent.z * glm::abs(glm::dot(plane->normal, obb->normals[2]));
        float dist = DistanceFromPointToPlane(obb->pos, plane->pos, plane->normal);
        return dist < -r;
    }

    bool CollisionShape::Intersect(OBB* a, OBB* b) {
        // TODO:
        const auto ab = b->pos - a->pos;
        const auto lengthAB = glm::length(ab);

        for (const auto& aNormal : a->normals) {

        }
        for (const auto& bNormal : b->normals) {

        }
        return true;
    }

    bool CollisionShape::Intersect(AABB* aabb, Triangle* triangle) {
        return false;
    }

    bool CollisionShape::Intersect(Sphere* sphere, Triangle* triangle) {
        // Find point P on triangle ABC closest to sphere center
        glm::vec3 p = ClosestPointToPointOnTriangle(sphere->pos, *triangle);

        // Sphere and triangle intersect if the (squared) distance from sphere
        // center to point p is less than the (squared) sphere radius
        glm::vec3 v = p - sphere->pos;
        return glm::dot(v, v) <= sphere->radius * sphere->radius;
    }

    bool CollisionShape::Intersect(Plane* plane, Triangle* triangle) {
        return false;
    }

    bool CollisionShape::Intersect(OBB* a, Triangle* triangle) {
        return false;
    }

    bool CollisionShape::Intersect(Triangle* a, Triangle* b) {
        return false;
    }

    AABB::AABB() {
        shape = Shape::AABB;
    }

    bool AABB::Intersect(CollisionShape* shape) {
        switch (shape->shape) {
        case Shape::AABB:
            return CollisionShape::Intersect(this, reinterpret_cast<AABB*>(shape));
        case Shape::Sphere:
            return CollisionShape::Intersect(this, reinterpret_cast<Sphere*>(shape));
        case Shape::Plane:
            return CollisionShape::Intersect(this, reinterpret_cast<Plane*>(shape));
        case Shape::Triangle:
            return CollisionShape::Intersect(this, reinterpret_cast<Triangle*>(shape));
        case Shape::Frustum:
            return shape->Intersect(this);
        case Shape::None:
            LOG_ERROR("Collision with none.");
            break;
        }

        return false;
    }

    Sphere::Sphere() {
        shape = Shape::Sphere;
    }

    bool Sphere::Intersect(CollisionShape* shape) {
        switch (shape->shape) {
        case Shape::AABB:
            return CollisionShape::Intersect(reinterpret_cast<AABB*>(shape), this);
        case Shape::Sphere:
            return CollisionShape::Intersect(this, reinterpret_cast<Sphere*>(shape));
        case Shape::Triangle:
            return CollisionShape::Intersect(this, reinterpret_cast<Triangle*>(shape));
        case Shape::Plane:
            return CollisionShape::Intersect(this, reinterpret_cast<Plane*>(shape));
        case Shape::Frustum:
            return shape->Intersect(this);
        case Shape::None:
            LOG_ERROR("Collision with none.");
            break;
        }

        return false;
    }

    Plane::Plane() {
        shape = Shape::Plane;
    }

    bool Plane::Intersect(CollisionShape* shape) {
        switch (shape->shape) {
        case Shape::AABB:
            return CollisionShape::Intersect(reinterpret_cast<AABB*>(shape), this);
        case Shape::Sphere:
            return CollisionShape::Intersect(reinterpret_cast<Sphere*>(shape), this);
        case Shape::Triangle:
            return CollisionShape::Intersect(this, reinterpret_cast<Triangle*>(shape));
        case Shape::Plane:
            return CollisionShape::Intersect(this, reinterpret_cast<Plane*>(shape));
        case Shape::Frustum:
            return shape->Intersect(this);
        case Shape::None:
            LOG_ERROR("Collision with none.");
            break;
        }

        return false;
    }

    OBB::OBB() {
        shape = Shape::OBB;
    }

    bool OBB::Intersect(CollisionShape* shape) {
        // TODO:
        return false;
    }

    Frustum::Frustum(CameraComponent& camera)
        : m_Camera(camera) {
        shape = Shape::Frustum;
        UpdateFrustum();
    }

    bool Frustum::Intersect(CollisionShape* shape) {
        for (uint32_t i = 0; i < 6; i++) {
            if (Faces[i].Intersect(shape))
                return false;
        }
        return true;
    }

    void Frustum::UpdateFrustum() {
        const float halfVSide = m_Camera.Far * tanf(m_Camera.FOV * .5f);
        const float halfHSide = halfVSide * m_Camera.Aspect;
        const glm::vec3 frontMultFar = m_Camera.Far * m_Camera.Forward;

        Faces[0].pos = m_Camera.Position + m_Camera.Near * m_Camera.Forward; // near
        Faces[0].normal = m_Camera.Forward;

        // TODO: probably wrong. copy paste from opengl project (right -> left hand coord system).
        Faces[1].pos = m_Camera.Position;  // left
        Faces[1].normal = glm::normalize(glm::cross(frontMultFar - m_Camera.Right * halfHSide, m_Camera.Up));

        Faces[2].pos = m_Camera.Position; // right
        Faces[2].normal = glm::normalize(glm::cross(m_Camera.Up, frontMultFar + m_Camera.Right * halfHSide));

        Faces[3].pos = m_Camera.Position; // bottom
        Faces[3].normal = glm::normalize(glm::cross(frontMultFar + m_Camera.Up * halfVSide, m_Camera.Right));

        Faces[4].pos = m_Camera.Position; // top
        Faces[4].normal = glm::normalize(glm::cross(m_Camera.Right, frontMultFar - m_Camera.Up * halfVSide));

        Faces[5].pos = m_Camera.Position + frontMultFar; // far
        Faces[5].normal = -m_Camera.Forward;
    }

    void Frustum::SetCamera(CameraComponent& camera) {
        m_Camera = camera;
    }

    Triangle::Triangle() {
        shape = Shape::Triangle;
    }

    bool Triangle::Intersect(CollisionShape* shape) {
        switch (shape->shape) {
        case Shape::AABB:
            return CollisionShape::Intersect(reinterpret_cast<AABB*>(shape), this);
        case Shape::Sphere:
            return CollisionShape::Intersect(reinterpret_cast<Sphere*>(shape), this);
        case Shape::Plane:
            return CollisionShape::Intersect(reinterpret_cast<Plane*>(shape), this);
        case Shape::Triangle:
            return CollisionShape::Intersect(reinterpret_cast<Triangle*>(shape), this);
        case Shape::Frustum:
            return shape->Intersect(this);
        case Shape::None:
            LOG_ERROR("Collision with none.");
            break;
        }

        return false;
    }
}