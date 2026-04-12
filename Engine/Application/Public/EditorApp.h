#pragma once

#include "Application/Public/ApplicationAPI.h"

#include <memory>

class ProjectApp;
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
	bool m_isEditorSessionActive = false;
};