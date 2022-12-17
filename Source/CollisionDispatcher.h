#ifndef FLOOF_COLLISIONDISPATCHER_H
#define FLOOF_COLLISIONDISPATCHER_H

#include "Scene.h"

namespace FLOOF{
    class CollisionDispatcher {
    public:
        CollisionDispatcher(Scene* scene, entt::entity& entity):mScene(scene),mEntity(entity){};

        virtual void onBeginOverlap(void* obj1, void*obj2) = 0;
        virtual void onOverlap(void* obj1, void* obj2){
            if(!isOverlapping){
                onBeginOverlap(obj1, obj2);
                isOverlapping = true;
            }};
        virtual void onEndOverlap(void* obj){
            isOverlapping = false;
        };

    protected:
        entt::entity& mEntity;
        bool isOverlapping = false;
        Scene* mScene;
    };
}



#endif //FLOOF_COLLISIONDISPATCHER_H
