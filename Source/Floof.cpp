#include "Application.h"

int main() {
    auto& app = FLOOF::Application::Get();
    int result = app.Run();
    return result;
}
