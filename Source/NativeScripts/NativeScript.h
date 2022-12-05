#pragma once
#include "entt/entt.hpp"
#include "../Scene.h"

namespace FLOOF {
    class Scene;
	class NativeScript{
	public:
		virtual void OnCreate(Scene* scene, entt::entity entity) { m_Scene = scene; m_Entity = entity; };
		virtual void OnUpdate(float deltaTime) {};
        virtual void LastUpdate(float deltaTime) {};
        virtual void EditorUpdate(float deltaTime){};

		entt::entity CreateEntity(const std::string& tag = "Entity", entt::entity parent = entt::null);

		template<typename Type>
		Type& GetComponent(entt::entity entity) {
			return m_Scene->GetComponent<Type>(entity);
		}

		template<typename Type, typename... Args>
		Type& AddComponent(entt::entity entity, Args &&...args) {
			return m_Scene->AddComponent<Type>(entity, std::forward<Args>(args)...);
		}

		template<typename Type>
		Type* TryGetComponent(entt::entity entity) {
			return m_Scene->TryGetComponent<Type>(entity);
		}

	protected:
		entt::entity m_Entity = entt::null;
		Scene* m_Scene = nullptr;

	};
}