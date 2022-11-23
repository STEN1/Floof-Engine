#ifndef FLOOF_ENGINEPANEL_H
#define FLOOF_ENGINEPANEL_H

#include "EditorPanel.h"
#include "../../Components.h"
#include "../../Math.h"
namespace FLOOF {
    class EnginePanel : public EditorPanel{
    public:
        EnginePanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
        virtual void DrawPanel();
    };
}

#endif //FLOOF_ENGINEPANEL_H
