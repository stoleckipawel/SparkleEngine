#pragma once

#include "ApplicationAPI.h"

#include <memory>

class ProjectApp;
class ShaderRecookCoordinator;
class UI;

class SPARKLE_APPLICATION_API EditorApp
{
  public:
	EditorApp();
	~EditorApp();

	EditorApp(const EditorApp&) = delete;
	EditorApp& operator=(const EditorApp&) = delete;
	EditorApp(EditorApp&&) = delete;
	EditorApp& operator=(EditorApp&&) = delete;

	void Initialize();
	bool Tick();
	void Shutdown();
	void Run();

	private:
	std::unique_ptr<ProjectApp> m_projectApp;
	std::unique_ptr<UI> m_ui;
	std::unique_ptr<ShaderRecookCoordinator> m_shaderRecookCoordinator;
	bool m_isEditorSessionActive = false;
};