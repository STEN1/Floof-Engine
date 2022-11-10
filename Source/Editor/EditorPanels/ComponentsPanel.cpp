#include "ComponentsPanel.h"
#include "../../Application.h"
#include <filesystem>

namespace FLOOF {
	void ComponentsPanel::DrawPanel()
	{
        auto& app = Application::Get();
        ImGui::Begin("Components");
        ImGui::Text("Components");
        if (app.m_Scene && app.m_Scene->m_SelectedEntity != entt::null) {
            if (auto* transform = app.m_Scene->GetRegistry().try_get<TransformComponent>(app.m_Scene->m_SelectedEntity)) {
                ImGui::Separator();
                ImGui::Text("Transform component");
                ImGui::DragFloat3("Position", &transform->Position[0], 0.1f);
                ImGui::DragFloat3("Rotation", &transform->Rotation[0]);

                if (auto* body = app.m_Scene->TryGetComponent<RigidBodyComponent>(app.m_Scene->m_SelectedEntity)) {
                    if (body->Primitive == bt::CollisionPrimitive::Sphere) {
                        ImGui::DragFloat("Scale", &transform->Scale[0]);
                        transform->Scale[1] = transform->Scale[2] = transform->Scale[0];
                    } else
                        ImGui::DragFloat3("Scale", &transform->Scale[0]);
                    //move physics body
                    body->transform(transform->Position, transform->Rotation, transform->Scale / 2.f);
                }


            }
            if (auto* rigidBody = app.m_Scene->GetRegistry().try_get<RigidBodyComponent>(app.m_Scene->m_SelectedEntity)) {
                ImGui::Separator();
                ImGui::Text("Rigid body component");
                if (rigidBody->RigidBody) {
                    ImGui::Text("Position: %.3f, %.3f, %.3f",
                        rigidBody->RigidBody->getCenterOfMassPosition().getX(),
                        rigidBody->RigidBody->getCenterOfMassPosition().getY(),
                        rigidBody->RigidBody->getCenterOfMassPosition().getZ());

                    ImGui::Text("Local Scale: %.3f, %.3f, %.3f",
                        rigidBody->CollisionShape->getLocalScaling().getX(),
                        rigidBody->CollisionShape->getLocalScaling().getY(),
                        rigidBody->CollisionShape->getLocalScaling().getZ());

                    ImGui::Text("Velocity: %.3f, %.3f, %.3f",
                        rigidBody->RigidBody->getLinearVelocity().getX(),
                        rigidBody->RigidBody->getLinearVelocity().getY(),
                        rigidBody->RigidBody->getLinearVelocity().getZ());

                    ImGui::Text("Velocity length: %.3f", rigidBody->RigidBody->getLinearVelocity().length());
                }
            }
            if (auto* softBody = app.m_Scene->GetRegistry().try_get<SoftBodyComponent>(app.m_Scene->m_SelectedEntity)) {
                ImGui::Separator();
                ImGui::Text("Soft body component");
            }
            if (auto* meshComponent = app.m_Scene->GetRegistry().try_get<MeshComponent>(app.m_Scene->m_SelectedEntity)) {
                ImGui::Separator();
                ImGui::Text("Mesh component");
                ImGui::Text(meshComponent->Data.Path.c_str());
            }
            if (auto* staticMeshComponent = app.m_Scene->GetRegistry().try_get<StaticMeshComponent>(app.m_Scene->m_SelectedEntity)) {
                ImGui::Separator();
                ImGui::Text("Static mesh component");
            }
            if (auto* texture = app.m_Scene->GetRegistry().try_get<TextureComponent>(app.m_Scene->m_SelectedEntity)) {
                ImGui::Separator();
                ImGui::Text("Texture component");
                ImGui::Text(texture->Data.Path.c_str());
                // TODO: Make imgui texture descriptor for all textures.
                //ImGui::Image(texture->Data.DesctriptorSet, ImVec2(50, 50));
            }
            if (auto* soundComponent = app.m_Scene->GetRegistry().try_get<SoundSourceComponent>(app.m_Scene->m_SelectedEntity)) {
                ImGui::Separator();
                ImGui::Text("Sound component");
            }
            if (auto* scriptComponent = app.m_Scene->TryGetComponent<ScriptComponent>(app.m_Scene->m_SelectedEntity)) {
                ImGui::Separator();
                ImGui::Text("Script Component");
                std::string currentscript = scriptComponent->ModuleName;
                currentscript.erase(0, 8);

                //todo this is bad, should not read all files every frame
                std::vector<std::string> scripts;
                for (const auto& entry : std::filesystem::directory_iterator("Scripts")) {
                    std::string cleanname = entry.path().string();
                    cleanname.erase(0, 8);
                    scripts.emplace_back(cleanname);

                }

                if (ImGui::BeginCombo("Active Script", currentscript.c_str())) {
                    for (int n = 0; n < scripts.size(); n++) {
                        bool is_selected = (currentscript == scripts[n]);
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                        if (ImGui::Selectable(scripts[n].c_str(), is_selected)) {
                            currentscript = scripts[n];
                            std::string path = "Scripts/";
                            path.append(currentscript);
                            scriptComponent->Script = path;
                            scriptComponent->ReloadScript();
                        }

                    }
                    ImGui::EndCombo();
                }

                if (ImGui::Button("Refresh Script")) {
                    scriptComponent->updateScripts();
                    scriptComponent->ReloadScript();
                }
                if (ImGui::Button("Run Script once")) {
                    scriptComponent->RunScript();
                }
            }

            ImGui::Begin("Scripts");
            if (ImGui::Button("Run all Scripts once")) {
                auto view = app.m_Scene->GetRegistry().view<ScriptComponent>();
                for (auto [entity, script] : view.each()) {
                    script.RunScript();
                }
            }
            if (ImGui::Button("Refresh all Scripts")) {
                auto view = app.m_Scene->GetRegistry().view<ScriptComponent>();
                for (auto [entity, script] : view.each()) {
                    script.updateScripts();
                    script.ReloadScript();
                }
            }
            // end Scripts
            ImGui::End();
        }
        // end Components
        ImGui::End();
	}
}