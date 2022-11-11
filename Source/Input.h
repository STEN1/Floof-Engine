#pragma once
#include <GLFW/glfw3.h>
#include "Floof.h"
#include "Math.h"
#include "imgui.h"

namespace FLOOF {
    class Input {
    public:
        static int Key(ImGuiKey key) {
            return ImGui::IsKeyDown(key);
        }
        static int MouseButton(ImGuiMouseButton button) {
            return ImGui::IsMouseDown(button);
        }
        static glm::vec2 MousePos() {
            auto pos = ImGui::GetMousePos();
            return glm::vec2(pos.x, pos.y);
        }
    };
}