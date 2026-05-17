#pragma once

#include "EditorAPI.h"
#include "../../Core/Public/Events/ScopedEventHandle.h"
#include "../../Renderer/Public/Diagnostics/RendererMemoryDiagnostics.h"
#include "../../Renderer/Public/Meshes/MeshDiagnostics.h"
#include "../../Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "../../Renderer/Public/Viewport/ViewportContracts.h"
#include "Scene/SceneObjectSelection.h"

#include <cstdint>
#include <functional>
#include <memory>

class Timer;
class MainMenuBarPanel;
class EditorConsoleSystem;
class SceneOutlinerPanel;
class SceneInspectorPanel;
class ViewportPanel;
class ViewportTopPanel;
class ProfilerPanel;
class UsedShadersPanel;
class UsedMeshesPanel;
class UsedTexturesPanel;
class InputSystem;
class LevelManager;
class GameScene;
class Window;
class RhiImGuiRenderer;
struct MeshPreviewGeometry;

struct EditorHostServices final
{
	Timer& RuntimeTimer;
	LevelManager* Levels = nullptr;
	GameScene* Scene = nullptr;
	RhiImGuiRenderer& ImGuiRenderer;
	Window& HostWindow;
	InputSystem& Input;
};

struct EditorDiagnosticsProviders final
{
	std::function<std::uint64_t()> ShaderPackageGeneration;
	std::function<MeshDiagnosticsSnapshot()> MeshDiagnostics;
	std::function<TextureDiagnosticsSnapshot()> TextureDiagnostics;
	std::function<RendererMemoryDiagnosticsSnapshot()> MemoryDiagnostics;
};

class SPARKLE_EDITOR_API UI final
{
  public:
	explicit UI(EditorHostServices hostServices);

	~UI() noexcept;

	UI(const UI&) = delete;
	UI& operator=(const UI&) = delete;
	UI(UI&&) = delete;
	UI& operator=(UI&&) = delete;

	const ViewportRenderRequest& GetViewportRenderRequest() const noexcept;
	void SetViewportRenderProducts(const ViewportRenderProducts& products) noexcept;
	void SetViewportSceneColorTextureId(std::uint64_t textureId) noexcept;
	void SetDiagnosticsProviders(EditorDiagnosticsProviders providers);
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	EditorConsoleSystem* GetEditorConsoleSystem() noexcept { return m_editorConsoleSystem.get(); }
	bool ConsumeShaderReloadRequest() noexcept;
	bool ConsumeShaderRecookRequest() noexcept;

	void Update();

	void Render() noexcept;

  private:
	void NewFrame();

	void Build();
	bool IsReady() const noexcept;

	void InitializeImGuiContext();

	bool InitializeWin32Backend();
	bool InitializeGraphicsBackend();

	void InitializeDefaultPanels();
	void ConfigureMainMenuBarWindowActions();
	MeshPreviewGeometry BuildMeshPreviewGeometry(std::uintptr_t meshRuntimeId) const;

	void SubscribeToWindowEvents(Window& window);

	void SetupDPIScaling() noexcept;

	std::unique_ptr<MainMenuBarPanel> m_mainMenuBar;
	std::unique_ptr<EditorConsoleSystem> m_editorConsoleSystem;
	std::unique_ptr<SceneOutlinerPanel> m_sceneOutlinerPanel;
	std::unique_ptr<SceneInspectorPanel> m_sceneInspectorPanel;
	std::unique_ptr<ViewportTopPanel> m_viewportTopPanel;
	std::unique_ptr<ViewportPanel> m_viewportPanel;
	std::unique_ptr<ProfilerPanel> m_profilerPanel;
	std::unique_ptr<UsedShadersPanel> m_usedShadersPanel;
	std::unique_ptr<UsedMeshesPanel> m_usedMeshesPanel;
	std::unique_ptr<UsedTexturesPanel> m_usedTexturesPanel;
	Timer* m_timer = nullptr;
	LevelManager* m_levelManager = nullptr;
	GameScene* m_gameScene = nullptr;
	RhiImGuiRenderer* m_imguiRenderer = nullptr;
	Window* m_window = nullptr;
	InputSystem* m_inputSystem = nullptr;
	SceneObjectSelection m_sceneSelection = SceneObjectSelection::None();
	std::function<std::uint64_t()> m_shaderPackageGenerationProvider;
	std::function<MeshDiagnosticsSnapshot()> m_meshDiagnosticsProvider;
	std::function<TextureDiagnosticsSnapshot()> m_textureDiagnosticsProvider;
	std::function<RendererMemoryDiagnosticsSnapshot()> m_memoryDiagnosticsProvider;
	bool m_shaderReloadRequested = false;
	bool m_shaderRecookRequested = false;
	bool m_isImGuiContextInitialized = false;
	bool m_isWin32BackendInitialized = false;
	bool m_isGraphicsBackendInitialized = false;

	ScopedEventHandle m_windowMessageHandle;
};
