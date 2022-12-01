#ifndef FLOOF_WONDERBAUMSCRIPT_H
#define FLOOF_WONDERBAUMSCRIPT_H

#include "NativeScript.h"

namespace FLOOF {
    class WonderBaumScript : public NativeScript {
    public:
        virtual void OnCreate(Scene *scene, entt::entity entity) override;
    };
}

#endif //FLOOF_WONDERBAUMSCRIPT_H
