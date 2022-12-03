#ifndef FLOOF_NETWORKPANEL_H
#define FLOOF_NETWORKPANEL_H

#include "EditorPanel.h"
namespace FLOOF{
    class NetworkPanel :public EditorPanel{
    public:
        NetworkPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
        virtual void DrawPanel() override;
    private:
        bool Active{false};
        int  Port{25565};
       std::string Ip{"255.255.255.255"};
       enum class Type{
           Server,
           Client
       };
       Type con;
    };
}



#endif //FLOOF_NETWORKPANEL_H
