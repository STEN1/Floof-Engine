#pragma once
#include "../Scene.h"

namespace FLOOF
{ 
	/// <summary>
	/// Temporarily gamemode class
	/// For easy c++ scripting
	/// Derived classes would ideally be made in a different project and dynamically linked
	/// </summary>
	class GameMode
	{
		friend class Application;
		
	protected:
		GameMode(FLOOF::Scene& scene) :m_Scene(scene) {};
		~GameMode() {};

		FLOOF::Scene& m_Scene;

		/// <summary>
		/// Called at application initialization
		/// </summary>
		virtual void OnCreate() {};

		/// <summary>
		/// Executed when begin play is pressed
		/// TODO: Implement
		/// </summary>
		virtual void OnBeginPlay() {};

		//TODO: Implement
		virtual void OnEndPlay() {};

		/// <summary>
		/// Updates during editor and in-game mode
		/// </summary>
		virtual void OnUpdateEditor(float deltaTime) {};

		/// <summary>
		/// Updates only in-game
		/// TODO: Implement
		/// </summary>
		virtual void OnUpdate(float deltaTime) {};
	};
}