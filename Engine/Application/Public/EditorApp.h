#pragma once

#include "Application/Public/ApplicationAPI.h"
#include "Renderer/Public/Overlays/RendererOverlay.h"

#include <memory>

class ProjectApp;

class SPARKLE_APPLICATION_API EditorApp
{
  public:
	explicit EditorApp(RendererOverlayFactory overlayFactory = {});
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
	RendererOverlayFactory m_overlayFactory;
	bool m_isEditorSessionActive = false;
};