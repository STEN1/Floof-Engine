

#include "EnginePanel.h"
#include "../../Application.h"
#include "../EditorLayer.h"

void FLOOF::EnginePanel::DrawPanel() {

    auto* scene = m_EditorLayer->GetScene();
    auto view = scene->GetCulledScene().view<EngineComponent>();

    ImGui::Begin("Engine Panel");
    ImGui::Text("Press 'F' to Toggle Headlights");
    ImGui::Text("Press 'Space' to Break");

    for (auto [entity, Engine]: view.each()) {
        if (ImGui::CollapsingHeader("Engine Graph")) {
            ImGui::PlotLines("Velocity", Engine.velocityGraph.data(), Engine.velocityGraph.size(),Engine.GraphOffset, "m/s", 0.0f, 50.f, ImVec2(200, 100.0f));
            ImGui::PlotLines("Torque", Engine.TorqueGraph.data(), Engine.TorqueGraph.size(),Engine.GraphOffset, "kn", 1000.0f, 10000.f, ImVec2(200, 100.0f));
        }
        bool transformChanged{false};
        if (ImGui::CollapsingHeader("Engine Controls")) {

            transformChanged |= ImGui::DragFloat("Max Velocity", &Engine.maxVelocity);
            transformChanged |= ImGui::DragFloat("Max Torque", &Engine.maxEngineForce);
            transformChanged |= ImGui::DragFloat("Max Breaking Force", &Engine.maxBreakingForce);

        }
        if (ImGui::CollapsingHeader("Suspension Control")) {
            transformChanged |= ImGui::DragFloat("Suspension Stiffness", &Engine.suspensionStiffness);
            transformChanged |= ImGui::DragFloat("Suspension Damping", &Engine.suspensionDamping);
            transformChanged |= ImGui::DragFloat("Suspension Length", &Engine.suspensionRestLength,0.01,0,3);
        }
        if (ImGui::CollapsingHeader("Wheel Controls")) {
            transformChanged |= ImGui::DragFloat("Wheel Friction", &Engine.WheelFriction,0.1,0.1,100);
        }
        if (ImGui::CollapsingHeader("Gearing")) {
            std::string gear = "Current Gear : ";
            gear += std::to_string(Engine.CurrentGear+1);
            ImGui::Text(gear.c_str());
        }

        if (transformChanged) {
            //remake stuff
            for (auto &hinge: Engine.axles) {
                // Drive engine.
                hinge->setMaxMotorForce(3, Engine.maxEngineForce);

                //suspension
                hinge->setDamping(2, Engine.suspensionDamping);
                hinge->setStiffness(2, Engine.suspensionStiffness);
                hinge->setLimit(2, 0, Engine.suspensionRestLength);

                hinge->getRigidBodyB().setFriction(Engine.WheelFriction);

            }

        }
    }

    ImGui::End();


}
