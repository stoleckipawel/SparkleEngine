#pragma once

#include "UI.h"
#include "Core/Public/Events/ScopedEventHandle.h"
#include "Scene/SceneObjectSelection.h"

#include <string>

class Timer;
class MainMenuBarPanel;
class EditorConsoleSystem;
class SceneOutlinerPanel;
class SceneInspectorPanel;
class ViewportPanel;
class ViewportToolbar;
class SettingsPanel;
class UsedShadersPanel;
class UsedMeshesPanel;
class UsedTexturesPanel;
class EngineRenderingSettingsController;
class EditorRestartService;
class InputSystem;
class LevelSession;
class Window;

class UI::Implementation final
{
public:
	explicit Implementation(EditorHostServices hostServices);
	~Implementation() noexcept;

	const ViewportRenderRequest& GetViewportRenderRequest() const noexcept;
	RenderViewCameraData UpdateViewportCamera(const CameraInputIntent& intent, float deltaSeconds) noexcept;
	void SetViewportToolbarActions(std::unique_ptr<ViewportToolbarActions> actions) noexcept;
	void SetViewportRenderProducts(const ViewportRenderProducts& products) noexcept;
	void SetViewportFinalColorTexture(UiTextureHandle texture) noexcept;
	void SetDiagnosticsProviders(EditorDiagnosticsProviders providers);
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	EditorConsoleSystem* GetEditorConsoleSystem() noexcept { return m_editorConsoleSystem.get(); }
	bool ConsumeShaderReloadRequest() noexcept;
	bool ConsumeShaderRecookRequest() noexcept;
	ViewportOutputAction ConsumeViewportOutputAction() noexcept;
	UiRenderPacket ConsumeRenderPacket();

	void Update();

private:
	void NewFrame();

	void Build();
	void BeginInputRouting(bool disableInteraction);
	float BuildMainMenuBar();
	void BuildSceneOutliner(bool disableInteraction, float mainMenuBarHeight);
	void BuildCenterWorkspace(bool disableInteraction, float mainMenuBarHeight);
	void BuildViewport(bool disableInteraction, float topInset, float bottomInset, float outlinerWidth, float inspectorWidth);
	void RegisterViewportInputRegion();
	void BuildSceneInspector(bool disableInteraction, float mainMenuBarHeight);
	void BuildUtilityPanels(bool disableInteraction);
	bool IsReady() const noexcept;

	void InitializeImGuiContext();

	bool InitializeWin32Backend();

	void InitializeWorkspace();
	void InitializeCorePanels();
	void InitializeViewport();
	void InitializeAssetPanels();
	void InitializeScenePanels();
	void UpdateSceneModel();
	void HandleTransactionShortcuts();
	void ConfigureMainMenuBarWindowActions();

	void SubscribeToWindowEvents(Window& window);

	void ApplyDpiScale(float dpiScale) noexcept;

	CVarControlExecutor m_consoleVariables;
	std::unique_ptr<MainMenuBarPanel> m_mainMenuBar;
	std::unique_ptr<EditorConsoleSystem> m_editorConsoleSystem;
	std::unique_ptr<SceneOutlinerPanel> m_sceneOutlinerPanel;
	std::unique_ptr<SceneInspectorPanel> m_sceneInspectorPanel;
	std::unique_ptr<ViewportToolbar> m_viewportToolbar;
	std::unique_ptr<ViewportPanel> m_viewportPanel;
	std::unique_ptr<class EditorViewportSession> m_viewportSession;
	std::unique_ptr<SettingsPanel> m_settingsPanel;
	std::unique_ptr<UsedShadersPanel> m_usedShadersPanel;
	std::unique_ptr<UsedMeshesPanel> m_usedMeshesPanel;
	std::unique_ptr<UsedTexturesPanel> m_usedTexturesPanel;
	std::unique_ptr<EngineRenderingSettingsController> m_renderingSettings;
	std::unique_ptr<EditorRestartService> m_restartService;
	Timer* m_timer = nullptr;
	LevelSession* m_levelSession = nullptr;
	Window* m_window = nullptr;
	InputSystem* m_inputSystem = nullptr;
	SceneObjectSelection m_sceneSelection = SceneObjectSelection::None();
	std::function<std::uint64_t()> m_shaderGenerationProvider;
	std::function<MeshDiagnosticsSnapshot()> m_meshDiagnosticsProvider;
	std::function<TextureDiagnosticsSnapshot()> m_textureDiagnosticsProvider;
	std::function<RendererMemoryDiagnosticsSnapshot()> m_memoryDiagnosticsProvider;
	std::function<MeshPreviewGeometry(std::uintptr_t)> m_meshPreviewProvider;
	std::unique_ptr<class EditorSceneModelBuilder> m_sceneModelBuilder;
	std::unique_ptr<class EditorTransactionHistory> m_transactionHistory;
	std::shared_ptr<const class EditorSceneModel> m_sceneModel;
	std::unique_ptr<class ImGuiRenderPacketBuilder> m_renderPacketBuilder;
	UiRenderPacket m_renderPacket;
	std::uint64_t m_viewportGeneration = 0;
	bool m_shaderReloadRequested = false;
	bool m_shaderRecookRequested = false;
	bool m_isImGuiContextInitialized = false;
	bool m_isWin32BackendInitialized = false;
	std::string m_imguiIniPath;

	ScopedEventHandle m_windowMessageHandle;
	ScopedEventHandle m_windowDpiScaleHandle;
};
