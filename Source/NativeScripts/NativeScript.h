#pragma once
#include "entt/entt.hpp"

namespace FLOOF {
	class Scene;
	class NativeScript {
	public:
		virtual void OnCreate(Scene* scene, entt::entity entity) { m_Scene = scene; m_Entity = entity; };
		virtual void OnUpdate(float deltaTime) {};
        virtual void OnOverlap(){
            if(!bIsOverlapping){
            OnBeginOverlap();
                bIsOverlapping = true;
        }}
        virtual void OnEndOverlap(){}; //todo implement in physicsystem
        virtual void OnBeginOverlap(){};
        virtual void LastUpdate(float deltaTime) {};
	protected:
		entt::entity m_Entity = entt::null;
		Scene* m_Scene = nullptr;

        bool bIsOverlapping{false};
	};
}