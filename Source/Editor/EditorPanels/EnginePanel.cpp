

#include "EnginePanel.h"
#include "../../Application.h"

void FLOOF::EnginePanel::DrawPanel() {

    auto scene = Application::Get().m_Scene;
    auto view = scene->GetCulledScene().view<EngineComponent>();

    ImGui::Begin("Engine Panel");

    for (auto [entity, Engine]: view.each()) {
        if (ImGui::CollapsingHeader("Engine Controls")) {
            bool transformChanged{false};
            transformChanged |= ImGui::DragFloat("Max Velocity", &Engine.maxVelocity);
            transformChanged |= ImGui::DragFloat("Max Torque", &Engine.maxEngineForce);
            transformChanged |= ImGui::DragFloat("Max Turn Torque", &Engine.maxTurnForce);
            transformChanged |= ImGui::DragFloat("Max Breaking Force", &Engine.maxBreakingForce);
            transformChanged |= ImGui::DragFloat("Suspension Stiffness", &Engine.suspensionStiffness);
            transformChanged |= ImGui::DragFloat("Suspension Damping", &Engine.suspensionDamping);


            if(transformChanged){
                //remake stuff
                for(auto& hinge: Engine.axles){
                    // Drive engine.
                    hinge->setMaxMotorForce(3, Engine.maxEngineForce);
                    // Steering engine.
                    hinge->setMaxMotorForce(5, Engine.maxTurnForce);
                    //suspension
                    hinge->setDamping( 2, Engine.suspensionDamping );
                    hinge->setStiffness( 2, Engine.suspensionStiffness);

                }
            }
        }
    }

    ImGui::End();


}
