#pragma once

#include "EditorAPI.h"
#include "../../Core/Public/Events/ScopedEventHandle.h"
#include "../../RHI/Public/Interop/RenderHardwareInterface.h"
#include "../../Renderer/Public/Viewport/ViewportContracts.h"
#include "Scene/SceneObjectSelection.h"

#include <Windows.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class Timer;
class MainMenuBarPanel;
class EditorConsoleSystem;
class SceneOutlinerPanel;
class SceneInspectorPanel;
class ViewportPanel;
class ProfilerPanel;
class UsedShadersPanel;
class LevelManager;
class GameScene;
class Window;
struct WindowMessageEvent;

class SPARKLE_EDITOR_API UI final
{
  public:
	UI(Timer& timer, LevelManager* levelManager, GameScene* gameScene, RenderHardwareInterface& renderHardware, Window& window);

	~UI() noexcept;

	UI(const UI&) = delete;
	UI& operator=(const UI&) = delete;
	UI(UI&&) = delete;
	UI& operator=(UI&&) = delete;

	void HandleWindowMessage(WindowMessageEvent& event) noexcept;

	bool ProcessWindowMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	const ViewportRenderRequest& GetViewportRenderRequest() const noexcept;
	bool WantsGameplayInput() const noexcept;
	void SetViewportRenderProducts(const ViewportRenderProducts& products) noexcept;
	void SetViewportSceneColorTextureId(std::uint64_t textureId) noexcept;
	void SetShaderPackageGenerationProvider(std::function<std::uint64_t()> provider);
	void SetShaderRecookStatus(std::string status);
	EditorConsoleSystem* GetEditorConsoleSystem() noexcept { return m_editorConsoleSystem.get(); }
	bool ConsumeShaderReloadRequest() noexcept;
	bool ConsumeShaderRecookRequest() noexcept;

	void Update();

	void Render(NativeGraphicsCommandListHandle commandList) noexcept;

  private:
	void NewFrame();

	void Build();
	void BuildShaderRecookStatusWindow(bool disableInteraction) noexcept;
	bool IsReady() const noexcept;

	void InitializeImGuiContext();

	bool InitializeWin32Backend();
	bool InitializeGraphicsBackend();

	bool InitializeNativeGraphicsBackend();

	void InitializeDefaultPanels();
	void ConfigureMainMenuBarShaderActions();

	void SubscribeToWindowEvents(Window& window);

	void SetupDPIScaling() noexcept;

	std::unique_ptr<MainMenuBarPanel> m_mainMenuBar;
	std::unique_ptr<EditorConsoleSystem> m_editorConsoleSystem;
	std::unique_ptr<SceneOutlinerPanel> m_sceneOutlinerPanel;
	std::unique_ptr<SceneInspectorPanel> m_sceneInspectorPanel;
	std::unique_ptr<ViewportPanel> m_viewportPanel;
	std::unique_ptr<ProfilerPanel> m_profilerPanel;
	std::unique_ptr<UsedShadersPanel> m_usedShadersPanel;
	Timer* m_timer = nullptr;
	LevelManager* m_levelManager = nullptr;
	GameScene* m_gameScene = nullptr;
	RenderHardwareInterface* m_renderHardware = nullptr;
	Window* m_window = nullptr;
	SceneObjectSelection m_sceneSelection = SceneObjectSelection::None();
	std::function<std::uint64_t()> m_shaderPackageGenerationProvider;
	std::string m_shaderRecookStatus;
	bool m_shaderReloadRequested = false;
	bool m_shaderRecookRequested = false;
	bool m_showShaderRecookStatus = false;
	bool m_isImGuiContextInitialized = false;
	bool m_isWin32BackendInitialized = false;
	bool m_isGraphicsBackendInitialized = false;

	ScopedEventHandle m_windowMessageHandle;
};
