#pragma once

#include "Application.h"
#include "../../Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>

class Timer;
class Window;
class InputSystem;
class GameWorld;
class Renderer;
class LevelSession;
class RuntimeConsoleHost;
class TaskExecutor;
class TaskScope;
class ApplicationTaskRuntime;
class CameraInputIntentCollector;
class EditorApplication;

enum class RuntimeApplicationFrameResult : std::uint8_t
{
	Exit = 0,
	SkipRender = 1,
	Ready = 2,
};

struct RuntimeApplicationOptions final
{
	bool EnableRuntimeConsole = true;
	bool AllowThreadedRenderer = true;
	bool EnableUiRenderPackets = false;
	bool EnableOscillatingMeshMotion = false;
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
	ViewportRenderProducts GetViewportRenderProducts() const;
	Timer& GetTimer() noexcept;
	Window& GetWindow() noexcept;
	InputSystem& GetInputSystem() noexcept;
	LevelSession* GetLevelSession() const noexcept;
	Renderer& GetRenderer() noexcept;
	TaskExecutor& GetTaskExecutor() noexcept;
	TaskScope& GetApplicationTaskScope() noexcept;
	bool Tick() override;
	void Shutdown() override;

  private:
	friend class EditorApplication;

	static bool WantsImGuiInputCapture() noexcept;
	void InitializeHost();
	void InitializeInput();
	void InitializeGameRuntime();
	void InitializeRenderer();
	void InitializeRuntimeConsole();
	void SubmitWorldRenderInput();
	GameWorld& GetWorldForEditor() noexcept;

	std::unique_ptr<Timer> m_timer;
	std::unique_ptr<Window> m_window;
	std::unique_ptr<InputSystem> m_inputSystem;
	std::unique_ptr<GameWorld> m_gameWorld;
	std::unique_ptr<LevelSession> m_levelSession;
	std::unique_ptr<Renderer> m_renderer;
	std::unique_ptr<RuntimeConsoleHost> m_runtimeConsoleHost;
	std::unique_ptr<ApplicationTaskRuntime> m_taskRuntime;
	std::unique_ptr<CameraInputIntentCollector> m_cameraInputIntentCollector;
	RuntimeApplicationOptions m_options;
	bool m_isInitialized = false;
};
