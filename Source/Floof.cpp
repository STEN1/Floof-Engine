#include "Application.h"
#include <filesystem>

//Argument for Application Layer, Default EditorLayer

int main(int argc, char *argv[]) {
#ifdef FLOOF_DEVELOPER_PATH
    std::filesystem::current_path(FLOOF_DEVELOPER_PATH);
#endif // FLOOF_DEVELOPER_PATH

    if(argc > 1){
        if(argv[1] == "server"){
            auto& app = FLOOF::Application::Get(FLOOF::Application::LayerType::SERVER);
            int result = app.Run();
            return result;
        }
        if(argv[1] == "editor"){
            auto& app = FLOOF::Application::Get(FLOOF::Application::LayerType::EDITOR);
            int result = app.Run();
            return result;
        }
    }
    else{
        auto& app = FLOOF::Application::Get(FLOOF::Application::LayerType::EDITOR);
        int result = app.Run();
        return result;
    }

}
