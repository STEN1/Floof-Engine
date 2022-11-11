#include "Application.h"
#include <filesystem>

int main() {
#ifdef FLOOF_DEVELOPER_PATH
    std::filesystem::current_path(FLOOF_DEVELOPER_PATH);
#endif // FLOOF_DEVELOPER_PATH

    auto& app = FLOOF::Application::Get();
    int result = app.Run();
    return result;
}
