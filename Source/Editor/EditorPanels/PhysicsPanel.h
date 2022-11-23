#ifndef FLOOF_PHYSICSPANEL_H
#define FLOOF_PHYSICSPANEL_H

#include "EditorPanel.h"
#include "../../Components.h"
#include "../../Math.h"

namespace FLOOF {
    class PhysicsPanel : public EditorPanel {
    public:
        PhysicsPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}

        virtual void DrawPanel();

        const entt::entity SpawnSoftMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath, const std::string texture);

        const entt::entity SpawnRigidMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath, const std::string texture, bt::CollisionPrimitive shape = bt::CollisionPrimitive::ConvexHull);

    };
}

#endif //FLOOF_PHYSICSPANEL_H
