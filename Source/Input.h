#pragma once
#include <GLFW/glfw3.h>
#include "Floof.h"
#include "Math.h"
#include <functional>
#include <unordered_map>

namespace FLOOF {
    class Input {
    public:
        static void Init(GLFWwindow* window) {
            s_Window = window;
            // overrides imgui key callbacks.
            //glfwSetKeyCallback(s_Window, GLFWKeyCallback);
        }
        static int Key(int keyCode) {
            return glfwGetKey(s_Window, keyCode);
        }
        static int MouseButton(int button) {
            return glfwGetMouseButton(s_Window, button);
        }
        static glm::vec2 MousePos() {
            double x, y;
            glfwGetCursorPos(s_Window, &x, &y);
            return glm::vec2(x, y);
        }
        static void RegisterKeyPressCallback(int keyCode, std::function<void()> function) {
            s_InputPressCallbacks[keyCode] = function;
        }
        static void RegisterKeyReleaseCallback(int keyCode, std::function<void()> function) {
            s_InputReleaseCallbacks[keyCode] = function;
        }
    private:
        static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
            switch (action) {
            case GLFW_PRESS:
            {
                auto it = s_InputPressCallbacks.find(key);
                if (it != s_InputPressCallbacks.end())
                    it->second();
            }
            case GLFW_RELEASE:
            {
                auto it = s_InputReleaseCallbacks.find(key);
                if (it != s_InputReleaseCallbacks.end())
                    it->second();
            }
            default:
                break;
            }
        }
        inline static GLFWwindow* s_Window = nullptr;
        inline static std::unordered_map<int, std::function<void()>> s_InputPressCallbacks;
        inline static std::unordered_map<int, std::function<void()>> s_InputReleaseCallbacks;
    };
}