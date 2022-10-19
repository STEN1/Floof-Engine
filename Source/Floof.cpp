#include "Application.h"

int main() {
    FLOOF::Application* app = new FLOOF::Application;
    int result = app->Run();
    delete app;
    return result;
}
