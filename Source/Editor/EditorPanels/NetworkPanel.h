#ifndef FLOOF_NETWORKPANEL_H
#define FLOOF_NETWORKPANEL_H

#include "EditorPanel.h"
namespace FLOOF{
    class NetworkPanel :public EditorPanel{
    public:
        NetworkPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
        virtual void DrawPanel() override;
    };
}



#endif //FLOOF_NETWORKPANEL_H
