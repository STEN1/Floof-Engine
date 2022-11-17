#include "ComponentsPanel.h"
#include "../../Application.h"
#include <filesystem>
#include <limits>

namespace FLOOF {
	void ComponentsPanel::DrawPanel() {
		auto& app = Application::Get();
		ImGui::Begin("Components");
		ImGui::Text("Components");
		if (app.m_Scene && app.m_Scene->m_SelectedEntity != entt::null) {
			if (auto* transform = app.m_Scene->GetRegistry().try_get<TransformComponent>(
				app.m_Scene->m_SelectedEntity)) {
				ImGui::Separator();
				ImGui::Text("Transform component");
                bool transformChanged {false};
                transformChanged |= ImGui::DragFloat3("Position", &transform->Position[0], 0.1f);
                transformChanged |= ImGui::DragFloat3("Rotation", &transform->Rotation[0]);

				if (auto* body = app.m_Scene->TryGetComponent<RigidBodyComponent>(app.m_Scene->m_SelectedEntity)) {
					if (body->Primitive == bt::CollisionPrimitive::Sphere) {
                        transformChanged |= ImGui::DragFloat("Scale", &transform->Scale[0], 0.1, 0.01, std::numeric_limits<float>::max());
						transform->Scale[1] = transform->Scale[2] = transform->Scale[0];
					} else
						transformChanged |= ImGui::DragFloat3("Scale", &transform->Scale[0], 0.1, 0.01, std::numeric_limits<float>::max());
					//move physics body
					if(transformChanged)
                        body->transform(transform->Position, transform->Rotation, transform->Scale / 2.f);
				} else {
                     ImGui::DragFloat3("Scale", &transform->Scale[0], 0.1, 0.01, std::numeric_limits<float>::max());
				}
			}
			if (auto* rigidBody = app.m_Scene->GetRegistry().try_get<RigidBodyComponent>(
				app.m_Scene->m_SelectedEntity)) {
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
			if (auto* staticMesh = app.m_Scene->GetRegistry().try_get<StaticMeshComponent>(app.m_Scene->m_SelectedEntity)) {
				ImGui::Separator();
				ImGui::Text("Static mesh component");
				if (ImGui::Button("Toggle all Wireframe off")) {
					for (auto& mesh : staticMesh->meshes) {
						staticMesh->mapDrawWireframeMeshes[mesh.MeshName] = false;
					}
				}
				if (app.m_Scene->m_SelectedEntity != app.m_Scene->m_LastSelectedEntity) {
					for (auto& mesh : staticMesh->meshes) {
						staticMesh->mapDrawWireframeMeshes[mesh.MeshName] = true;
					}
				}
				ImGui::Text("Internal Meshes list:");
				for (auto& mesh : staticMesh->meshes) {
					std::string name = "\t";
					name += mesh.MeshName;
					if (ImGui::Selectable(name.c_str(), staticMesh->mapDrawWireframeMeshes[mesh.MeshName])) {
						staticMesh->mapDrawWireframeMeshes[mesh.MeshName] = !staticMesh->mapDrawWireframeMeshes[mesh.MeshName];
					}
				}

				if (auto* soundComponent = app.m_Scene->GetRegistry().try_get<SoundSourceComponent>(
					app.m_Scene->m_SelectedEntity)) {
					ImGui::Separator();
					ImGui::Text("Sound Component");
					ImGui::Text(soundComponent->m_Path.c_str());

					if (ImGui::DragFloat("Volume", &soundComponent->m_Volume, 0.001f, 0.f, 1.f)) {
						soundComponent->Update();
					}
					if (ImGui::DragFloat("Pitch", &soundComponent->m_Pitch, 0.01f, 0.f, 50.f)) {
						soundComponent->Pitch();
					}

					if (soundComponent->isPlaying) { if (ImGui::Button("Stop")) { soundComponent->Stop(); } }
					if (!soundComponent->isPlaying) { if (ImGui::Button("Play")) { soundComponent->Play(); } }
					if (soundComponent->isLooping) { if (ImGui::Button("Looping")) { soundComponent->Looping(false); } }
					if (!soundComponent->isLooping) { if (ImGui::Button("Not Looping")) { soundComponent->Looping(true); } }

				}

				ImGui::Text("Materials");
				ImVec2 imageSize(200.f, 200.f);
				for (auto& mesh : staticMesh->meshes) {
					ImGui::Separator();
					ImGui::Text("Diffuse");
					ImGui::Image(mesh.MeshMaterial.Diffuse.VkTexture.DesctriptorSet, imageSize);
					ImGui::Text("Normals");
					ImGui::Image(mesh.MeshMaterial.Normals.VkTexture.DesctriptorSet, imageSize);
					ImGui::Text("Metallic");
					ImGui::Image(mesh.MeshMaterial.Metallic.VkTexture.DesctriptorSet, imageSize);
					ImGui::Text("Roughness");
					ImGui::Image(mesh.MeshMaterial.Roughness.VkTexture.DesctriptorSet, imageSize);
					ImGui::Text("AO");
					ImGui::Image(mesh.MeshMaterial.AO.VkTexture.DesctriptorSet, imageSize);
				}
			

			}
			if (auto* scriptComponent = app.m_Scene->TryGetComponent<ScriptComponent>(
				app.m_Scene->m_SelectedEntity)) {
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
					scriptComponent->ReloadScript();
				}
				if (ImGui::Button("Run Script once")) {
					scriptComponent->RunScript();
				}
			}
		}
		// end Components
		ImGui::End();
	}
}