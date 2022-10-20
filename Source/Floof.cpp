#include "Application.h"

int main() {
    FLOOF::Application& app = FLOOF::Application::Get();
    int result = app.Run();
    return result;
}
