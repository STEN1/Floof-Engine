#include "Application.h"

int main() {
    // Need to be explicit here so the application is destroyed
    // before static destruction of static managers.
    FLOOF::Application* app = new FLOOF::Application();
    int result = app->Run();
    delete app;
    return result;
}
