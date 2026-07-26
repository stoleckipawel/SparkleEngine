#pragma once

#include "EditorAPI.h"
#include "Renderer/Public/Diagnostics/MeshPreviewGeometry.h"
#include "../../Core/Public/Events/ScopedEventHandle.h"
#include "../../Renderer/Public/Diagnostics/RendererMemoryDiagnostics.h"
#include "../../Renderer/Public/Editor/EditorRenderPacket.h"
#include "../../Renderer/Public/Settings/EngineRenderingSettings.h"
#include "../../Renderer/Public/Meshes/MeshDiagnostics.h"
#include "../../Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "../../Renderer/Public/Viewport/ViewportContracts.h"
#include "Scene/SceneObjectSelection.h"
#include "../../GameFramework/Public/World/WorldChange.h"
#include "../../GameFramework/Public/World/WorldEditCommand.h"
#include "../../GameFramework/Public/World/WorldMaterialVariantView.h"
#include "../../GameFramework/Public/World/WorldReadView.h"

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
class SettingsPanel;
class UsedShadersPanel;
class UsedMeshesPanel;
class UsedTexturesPanel;
class EngineRenderingSettingsSection;
class EditorRestartService;
class InputSystem;
class LevelManager;
class Window;

struct EditorHostServices final
{
	Timer& RuntimeTimer;
	LevelManager* Levels = nullptr;
	std::function<WorldReadView()> AcquireWorldReadView;
	std::function<WorldChangeBatch(const WorldChangeCursor&)> ReadWorldChanges;
	std::function<bool(WorldChangeCursor&, WorldSequence)> AcknowledgeWorldChanges;
	std::function<std::uint64_t()> WorldGeneration;
	std::function<WorldMaterialVariantView()> MaterialVariants;
	std::function<WorldEditResult(WorldEditCommand, std::uint64_t)> SubmitWorldEdit;
	std::function<void(EngineRenderingSettingsState)> SubmitRenderingSettings;
	Window& HostWindow;
	InputSystem& Input;
};

struct EditorDiagnosticsProviders final
{
	std::function<std::uint64_t()> ShaderPackageGeneration;
	std::function<MeshDiagnosticsSnapshot()> MeshDiagnostics;
	std::function<TextureDiagnosticsSnapshot()> TextureDiagnostics;
	std::function<RendererMemoryDiagnosticsSnapshot()> MemoryDiagnostics;
	std::function<MeshPreviewGeometry(std::uintptr_t)> MeshPreview;
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
	void SetViewportSceneColorTextureId(EditorTextureHandle texture) noexcept;
	void SetDiagnosticsProviders(EditorDiagnosticsProviders providers);
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	EditorConsoleSystem* GetEditorConsoleSystem() noexcept { return m_editorConsoleSystem.get(); }
	bool ConsumeShaderReloadRequest() noexcept;
	bool ConsumeShaderRecookRequest() noexcept;
	bool ConsumeViewportCaptureRequest() noexcept;
	EditorRenderPacket ConsumeRenderPacket();

	void Update();

  private:
	void NewFrame();

	void Build();
	void BeginInputRouting(bool disableInteraction);
	float BuildMainMenuBar();
	void BuildSceneOutliner(bool disableInteraction, float mainMenuBarHeight);
	void BuildCenterWorkspace(bool disableInteraction, float mainMenuBarHeight);
	void BuildViewport(
	    bool disableInteraction,
	    float topInset,
	    float bottomInset,
	    float outlinerWidth,
	    float inspectorWidth);
	void RegisterViewportInputRegion();
	void BuildSceneInspector(bool disableInteraction, float mainMenuBarHeight);
	void BuildUtilityPanels(bool disableInteraction);
	bool IsReady() const noexcept;

	void InitializeImGuiContext();

	bool InitializeWin32Backend();

	void InitializeDefaultPanels();
	void UpdateSceneModel();
	void HandleTransactionShortcuts();
	void ConfigureMainMenuBarWindowActions();

	void SubscribeToWindowEvents(Window& window);

	void SetupDPIScaling() noexcept;

	std::unique_ptr<MainMenuBarPanel> m_mainMenuBar;
	std::unique_ptr<EditorConsoleSystem> m_editorConsoleSystem;
	std::unique_ptr<SceneOutlinerPanel> m_sceneOutlinerPanel;
	std::unique_ptr<SceneInspectorPanel> m_sceneInspectorPanel;
	std::unique_ptr<ViewportTopPanel> m_viewportTopPanel;
	std::unique_ptr<ViewportPanel> m_viewportPanel;
	std::unique_ptr<SettingsPanel> m_settingsPanel;
	std::unique_ptr<UsedShadersPanel> m_usedShadersPanel;
	std::unique_ptr<UsedMeshesPanel> m_usedMeshesPanel;
	std::unique_ptr<UsedTexturesPanel> m_usedTexturesPanel;
	std::unique_ptr<EngineRenderingSettingsSection> m_renderingSettings;
	std::unique_ptr<EditorRestartService> m_restartService;
	Timer* m_timer = nullptr;
	LevelManager* m_levelManager = nullptr;
	Window* m_window = nullptr;
	InputSystem* m_inputSystem = nullptr;
	SceneObjectSelection m_sceneSelection = SceneObjectSelection::None();
	std::function<std::uint64_t()> m_shaderPackageGenerationProvider;
	std::function<MeshDiagnosticsSnapshot()> m_meshDiagnosticsProvider;
	std::function<TextureDiagnosticsSnapshot()> m_textureDiagnosticsProvider;
	std::function<RendererMemoryDiagnosticsSnapshot()> m_memoryDiagnosticsProvider;
	std::function<MeshPreviewGeometry(std::uintptr_t)> m_meshPreviewProvider;
	std::unique_ptr<class EditorSceneModelBuilder> m_sceneModelBuilder;
	std::unique_ptr<class EditorTransactionManager> m_transactions;
	std::shared_ptr<const class EditorSceneModel> m_sceneModel;
	std::unique_ptr<class EditorRenderPacketBuilder> m_renderPacketBuilder;
	EditorRenderPacket m_renderPacket;
	std::uint64_t m_viewportGeneration = 0;
	bool m_shaderReloadRequested = false;
	bool m_shaderRecookRequested = false;
	bool m_viewportCaptureRequested = false;
	bool m_isImGuiContextInitialized = false;
	bool m_isWin32BackendInitialized = false;

	ScopedEventHandle m_windowMessageHandle;
};
