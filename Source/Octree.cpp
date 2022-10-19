#include "Octree.h"
#include "Physics.h"

namespace FLOOF {
    Octree::Octree(const AABB& aabb)
        : m_AABB(aabb) {
    }

    void Octree::Insert(std::shared_ptr<CollisionObject> object) {
        if (!object->Shape->Intersect(&m_AABB))
            return;

        if (IsLeaf()) {
            // intersecting with node and node is leaf. insert.

            m_CollisionObjects.push_back(object);
            m_IsActive = true;

            if (m_CollisionObjects.size() > s_MaxObjects && m_AABB.extent.x > s_MinExtent) {
                // to many objects in node and extent is larger than min extent.
                Divide();
                for (auto& node : m_ChildNodes) {
                    for (auto& obj : m_CollisionObjects) {
                        node->Insert(obj);
                    }
                }
                m_CollisionObjects.clear();
            }
        } else {
            // not a leaf. send down.
            for (auto& node : m_ChildNodes) {
                node->Insert(object);
            }
        }
    }

    void Octree::FindIntersectingObjects(const CollisionObject& object, std::vector<CollisionObject*>& outVec) {
        for (auto& node : m_ChildNodes) {
            if (node->m_IsActive)
                node->FindIntersectingObjects(object, outVec);
        }

        if (IsLeaf()) {
            for (auto& obj : m_CollisionObjects) {
                if (obj->Shape == object.Shape) {
                    continue;
                }

                if (obj->Shape->Intersect(object.Shape)) {
                    bool found = false;
                    for (auto outObject : outVec) {
                        if (obj->Shape == outObject->Shape) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        continue;
                    }
                    outVec.push_back(obj.get());
                }
            }
        }
    }

    void Octree::GetCollisionPairs(std::vector<std::pair<CollisionObject*, CollisionObject*>>& outVec) {
        for (auto& node : m_ChildNodes) {
            if (node->m_IsActive)
                node->GetCollisionPairs(outVec);
        }

        if (IsLeaf()) {
            for (int i = 0; i < (int)m_CollisionObjects.size() - 1; i++) {
                for (int j = i + 1; j < m_CollisionObjects.size(); j++) {
                    // Continue if not intersecting.
                    if (!m_CollisionObjects[i]->Shape->Intersect(m_CollisionObjects[j]->Shape)) {
                        continue;
                    }

                    // Look for j in i. i should then also be in j so dont need to check.
                    auto it = std::find(m_CollisionObjects[i]->OverlappingShapes.begin(),
                        m_CollisionObjects[i]->OverlappingShapes.end(), m_CollisionObjects[i]->Shape);

                    // Continue if found.
                    if (it != m_CollisionObjects[i]->OverlappingShapes.end()) {
                        continue;
                    }

                    // We are now intersecting and not in overlapping arrays.
                    // In each overlapping array and push to outVec.
                    m_CollisionObjects[i]->OverlappingShapes.push_back(m_CollisionObjects[j]->Shape);
                    m_CollisionObjects[j]->OverlappingShapes.push_back(m_CollisionObjects[i]->Shape);

                    outVec.emplace_back(std::make_pair(m_CollisionObjects[i].get(), m_CollisionObjects[j].get()));
                }
            }
        }
    }

    bool Octree::IsLeaf() {
        if (m_ChildNodes.empty())
            return true;
        return false;
    }

    void Octree::Divide() {
        AABB a = m_AABB;
        a.extent /= 2.f;
        a.pos.x -= a.extent.x;
        a.pos.z -= a.extent.z;
        a.pos.y -= a.extent.y;
        AABB b = m_AABB;
        b.extent /= 2.f;
        b.pos.x -= b.extent.x;
        b.pos.z += b.extent.z;
        b.pos.y -= b.extent.y;
        AABB c = m_AABB;
        c.extent /= 2.f;
        c.pos.x += c.extent.x;
        c.pos.z += c.extent.z;
        c.pos.y -= c.extent.y;
        AABB d = m_AABB;
        d.extent /= 2.f;
        d.pos.x += d.extent.x;
        d.pos.z -= d.extent.z;
        d.pos.y -= d.extent.y;

        AABB e = m_AABB;
        e.extent /= 2.f;
        e.pos.x -= e.extent.x;
        e.pos.z -= e.extent.z;
        e.pos.y += e.extent.y;
        AABB f = m_AABB;
        f.extent /= 2.f;
        f.pos.x -= f.extent.x;
        f.pos.z += f.extent.z;
        f.pos.y += f.extent.y;
        AABB g = m_AABB;
        g.extent /= 2.f;
        g.pos.x += g.extent.x;
        g.pos.z += g.extent.z;
        g.pos.y += g.extent.y;
        AABB h = m_AABB;
        h.extent /= 2.f;
        h.pos.x += h.extent.x;
        h.pos.z -= h.extent.z;
        h.pos.y += h.extent.y;

        m_ChildNodes.reserve(8);
        m_ChildNodes.emplace_back(std::make_unique<Octree>(a));
        m_ChildNodes.emplace_back(std::make_unique<Octree>(b));
        m_ChildNodes.emplace_back(std::make_unique<Octree>(c));
        m_ChildNodes.emplace_back(std::make_unique<Octree>(d));
        m_ChildNodes.emplace_back(std::make_unique<Octree>(e));
        m_ChildNodes.emplace_back(std::make_unique<Octree>(f));
        m_ChildNodes.emplace_back(std::make_unique<Octree>(g));
        m_ChildNodes.emplace_back(std::make_unique<Octree>(h));
    }

    void Octree::GetActiveLeafNodes(std::vector<Octree*>& outVec) {
        for (auto& node : m_ChildNodes) {
            if (node->m_IsActive)
                node->GetActiveLeafNodes(outVec);
        }

        if (IsLeaf()) {
            outVec.push_back(this);
        }
    }

    void Octree::GetAllNodes(std::vector<Octree*>& outVec) {
        for (auto& node : m_ChildNodes) {
            node->GetAllNodes(outVec);
        }

        outVec.push_back(this);
    }
}