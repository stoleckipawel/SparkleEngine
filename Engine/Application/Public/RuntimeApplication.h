#pragma once

#include "Application.h"
#include "../../Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>
#include <functional>
#include <memory>

class Timer;
class Window;
class InputSystem;
class GameScene;
class Renderer;
class LevelManager;
class RuntimeConsoleHost;
namespace Assets
{
	class SceneAssetManager;
}

enum class RuntimeApplicationFrameResult : std::uint8_t
{
	Exit = 0,
	SkipRender = 1,
	Ready = 2,
};

struct RuntimeApplicationOptions final
{
	bool EnableRuntimeConsole = true;
	std::function<void(GameScene&)> SceneSetupCallback;
};

class SPARKLE_APPLICATION_API RuntimeApplication final : public Application
{
  public:
	RuntimeApplication();
	explicit RuntimeApplication(RuntimeApplicationOptions options) noexcept;
	~RuntimeApplication();

	RuntimeApplication(const RuntimeApplication&) = delete;
	RuntimeApplication& operator=(const RuntimeApplication&) = delete;
	RuntimeApplication(RuntimeApplication&&) = delete;
	RuntimeApplication& operator=(RuntimeApplication&&) = delete;

	void Initialize() override;
	RuntimeApplicationFrameResult BeginFrame();
	void UpdateRuntime() noexcept;
	void SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept;
	const ViewportRenderProducts& GetViewportRenderProducts() const noexcept;
	void EndFrame() noexcept;
	Timer& GetTimer() noexcept;
	Window& GetWindow() noexcept;
	InputSystem& GetInputSystem() noexcept;
	GameScene* GetGameScene() const noexcept;
	LevelManager* GetLevelManager() const noexcept;
	Renderer& GetRenderer() noexcept;
	bool Tick() override;
	void Shutdown() override;

  private:
	std::unique_ptr<Timer> m_timer;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<InputSystem> m_inputSystem;
	std::unique_ptr<GameScene> m_gameScene;
	std::unique_ptr<Assets::SceneAssetManager> m_sceneAssetManager;
	std::unique_ptr<LevelManager> m_levelManager;
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<RuntimeConsoleHost> m_runtimeConsoleHost;
	RuntimeApplicationOptions m_options;
	bool m_isInitialized = false;
};
