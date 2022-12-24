#include "Application.h"
#include <filesystem>

//Argument for Application Layer, Default EditorLayer

int main(int argc, char *argv[]) {
#ifdef FLOOF_DEVELOPER_PATH
    std::filesystem::current_path(FLOOF_DEVELOPER_PATH);
#endif // FLOOF_DEVELOPER_PATH

    std::cout << "You have entered " << argc
         << " arguments:" << "\n";

    for (int i = 0; i < argc; ++i)
        std::cout << argv[i] << "\n";

    if(argc > 1){
        if(strcmp(argv[1], "server") == 0){
            std::cout << "Starting Floof Server " << std::endl;
            auto& app = FLOOF::Application::SetLayerType(FLOOF::LayerType::SERVER);
            int result = app.Run();
            return result;
        }
        else if(strcmp(argv[1], "editor") == 0){
            std::cout << "Starting Floof Editor " << std::endl;
            auto& app = FLOOF::Application::SetLayerType(FLOOF::LayerType::EDITOR);
            int result = app.Run();
            return result;
        }
    }
    else{
        auto& app = FLOOF::Application::SetLayerType(FLOOF::LayerType::EDITOR);
        int result = app.Run();
        return result;
    }

}
