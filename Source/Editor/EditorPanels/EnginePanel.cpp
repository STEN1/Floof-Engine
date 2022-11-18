

#include "EnginePanel.h"
#include "../../Application.h"

void FLOOF::EnginePanel::DrawPanel() {

    auto scene = Application::Get().m_Scene;
    auto view = scene->GetCulledScene().view<EngineComponent>();

    ImGui::Begin("Engine Panel");

    for (auto [entity, Engine]: view.each()) {
        bool transformChanged{false};
        if (ImGui::CollapsingHeader("Engine Controls")) {

            transformChanged |= ImGui::DragFloat("Max Velocity", &Engine.maxVelocity);
            transformChanged |= ImGui::DragFloat("Max Torque", &Engine.maxEngineForce);
            transformChanged |= ImGui::DragFloat("Max Turn Torque", &Engine.maxTurnForce);
            transformChanged |= ImGui::DragFloat("Max Breaking Force", &Engine.maxBreakingForce);

        }
        if (ImGui::CollapsingHeader("Suspension Control")) {
            transformChanged |= ImGui::DragFloat("Suspension Stiffness", &Engine.suspensionStiffness);
            transformChanged |= ImGui::DragFloat("Suspension Damping", &Engine.suspensionDamping);
            transformChanged |= ImGui::DragFloat("Suspension Length", &Engine.suspensionRestLength,0.01,0,3);
        }
        if (ImGui::CollapsingHeader("Wheel Controls")) {
            transformChanged |= ImGui::DragFloat("Wheel Friction", &Engine.WheelFriction,0.1,0.1,100);
            transformChanged |= ImGui::DragFloat("Rolling Friction", &Engine.RollingFriction,0.1,0.1,100);
            transformChanged |= ImGui::DragFloat("Spinning Friction", &Engine.SpinningFriction,0.1,0.1,100);
        }

        if (transformChanged) {
            //remake stuff
            for (auto &hinge: Engine.axles) {
                // Drive engine.
                hinge->setMaxMotorForce(3, Engine.maxEngineForce);
                // Steering engine.
                hinge->setMaxMotorForce(5, Engine.maxTurnForce);
                //suspension
                hinge->setDamping(2, Engine.suspensionDamping);
                hinge->setStiffness(2, Engine.suspensionStiffness);
                hinge->setLimit(2, -Engine.suspensionRestLength/2.f, Engine.suspensionRestLength/2.f);

                hinge->getRigidBodyB().setFriction(Engine.WheelFriction);
                hinge->getRigidBodyB().setRollingFriction(Engine.RollingFriction);
                hinge->getRigidBodyB().setSpinningFriction(Engine.SpinningFriction);

            }

        }
    }

    ImGui::End();


}
