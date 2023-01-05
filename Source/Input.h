#pragma once
#include <GLFW/glfw3.h>
#include "Floof.h"
#include "Math.h"
#include "imgui.h"
#include "Application.h"

namespace FLOOF {
    class Input {
    public:
        static int Key(ImGuiKey key) {
            if(Application::Get().GetLayerType() != LayerType::SERVER) {
                return ImGui::IsKeyDown(key);
            }
            else return 0;
        }
        static int MouseButton(ImGuiMouseButton button) {

            if(Application::Get().GetLayerType() != LayerType::SERVER) {
            return ImGui::IsMouseDown(button);
            }
            else return 0;
        }
        static glm::vec2 MousePos() {
            if(Application::Get().GetLayerType() != LayerType::SERVER) {
            auto pos = ImGui::GetMousePos();
            return glm::vec2(pos.x, pos.y);
            }
            else return glm::vec2(0.f);
        }
    };
}