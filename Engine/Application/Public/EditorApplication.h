#pragma once

#include "Application.h"
#include "RuntimeApplication.h"

#include <memory>

class RuntimeApplication;
class ShaderRecookCoordinator;
class EditorViewportCaptureCoordinator;
class EditorOperationService;
class GameWorld;
class Renderer;
class UI;
struct EditorHostServices;

class SPARKLE_APPLICATION_API EditorApplication final : public Application
{
  public:
	EditorApplication();
	explicit EditorApplication(RuntimeApplicationOptions options) noexcept;
	~EditorApplication();

	EditorApplication(const EditorApplication&) = delete;
	EditorApplication& operator=(const EditorApplication&) = delete;
	EditorApplication(EditorApplication&&) = delete;
	EditorApplication& operator=(EditorApplication&&) = delete;

	void Initialize() override;
	bool Tick() override;
	void Shutdown() override;

  private:
	void InitializeRuntimeApplication();
	void InitializeEditorOperations();
	void InitializeUi();
	EditorHostServices BuildUiHostServices(
	    Renderer& renderer,
	    GameWorld& world);
	void ConfigureUiDiagnostics(Renderer& renderer);
	void UpdateEditorOperations(Renderer& renderer);
	void RenderEditorFrame(Renderer& renderer);

	std::unique_ptr<RuntimeApplication> m_runtimeApplication;
	std::unique_ptr<UI> m_ui;
	std::unique_ptr<ShaderRecookCoordinator> m_shaderRecookCoordinator;
	std::unique_ptr<EditorViewportCaptureCoordinator> m_viewportCaptureCoordinator;
	std::unique_ptr<EditorOperationService> m_operationService;
	RuntimeApplicationOptions m_runtimeOptions;
	bool m_isEditorSessionActive = false;
};
