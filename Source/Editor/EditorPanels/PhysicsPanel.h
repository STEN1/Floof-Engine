#ifndef FLOOF_PHYSICSPANEL_H
#define FLOOF_PHYSICSPANEL_H

#include "EditorPanel.h"
#include "../../Components.h"
#include "../../Math.h"

namespace FLOOF {
    class PhysicsPanel : public EditorPanel {
    public:
        virtual void DrawPanel();

        static const entt::entity SpawnSoftMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath, const std::string texture);

        static const entt::entity SpawnRigidMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath, const std::string texture, bt::CollisionPrimitive shape = bt::CollisionPrimitive::ConvexHull);

    };
}

#endif //FLOOF_PHYSICSPANEL_H
