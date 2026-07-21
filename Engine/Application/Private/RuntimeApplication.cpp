#include "PCH.h"
#include "RuntimeApplication.h"

#include "Window/Window.h"
#include "Renderer.h"
#include "RuntimeConsole/RuntimeConsoleHost.h"
#include "Input/InputSystem.h"
#include "World/GameWorld.h"
#include "Input/CameraInputIntentCollector.h"
#include "Level/LevelManager.h"
#include "Time/Timer.h"
#include "Concurrency/ApplicationTaskRuntime.h"


#include <imgui.h>

namespace
{
	bool WantsImGuiInputCapture() noexcept
	{
		const ImGuiContext* currentContext = ImGui::GetCurrentContext();
		return currentContext != nullptr && (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse);
	}
}

RuntimeApplication::RuntimeApplication() = default;

RuntimeApplication::RuntimeApplication(RuntimeApplicationOptions options) noexcept : m_options(options) {}

RuntimeApplication::~RuntimeApplication() = default;

Timer& RuntimeApplication::GetTimer() noexcept
{
	return *m_timer;
}

Window& RuntimeApplication::GetWindow() noexcept
{
	return *m_window;
}

InputSystem& RuntimeApplication::GetInputSystem() noexcept
{
	return *m_inputSystem;
}

GameWorld* RuntimeApplication::GetGameWorld() const noexcept
{
	return m_gameWorld.get();
}

LevelManager* RuntimeApplication::GetLevelManager() const noexcept
{
	return m_levelManager.get();
}

Renderer& RuntimeApplication::GetRenderer() noexcept
{
	return *m_renderer;
}

TaskExecutor& RuntimeApplication::GetTaskExecutor() noexcept { return m_taskRuntime->GetExecutor(); }

TaskScope& RuntimeApplication::GetApplicationTaskScope() noexcept { return m_taskRuntime->GetApplicationScope(); }

void RuntimeApplication::Initialize()
{
	if (m_isInitialized)
	{
		return;
	}

	{
		m_timer = std::make_unique<Timer>();
		m_window = std::make_unique<Window>("Sparkle Engine");
	}

	{
		m_inputSystem = InputSystem::Create();
		m_inputSystem->SetInputCaptureQuery(WantsImGuiInputCapture);
		m_inputSystem->SubscribeToWindow(*m_window);
	}

	{
		m_taskRuntime = std::make_unique<ApplicationTaskRuntime>();
		m_gameWorld = std::make_unique<GameWorld>(m_taskRuntime->GetExecutor());
		m_cameraInputIntentCollector = std::make_unique<CameraInputIntentCollector>(*m_inputSystem, *m_window);
		if (m_options.WorldSetupCallback)
		{
			m_options.WorldSetupCallback(*m_gameWorld);
		}
		m_levelManager = std::make_unique<LevelManager>(
		    *m_gameWorld,
		    m_taskRuntime->GetExecutor(),
		    m_taskRuntime->GetApplicationScope());
	}

	{
		m_renderer = std::make_unique<Renderer>(*m_timer, *m_window);
	}

	if (m_options.EnableRuntimeConsole)
	{
		m_runtimeConsoleHost = std::make_unique<RuntimeConsoleHost>(*m_timer, *m_window, *m_renderer);
	}
	m_isInitialized = true;
}

RuntimeApplicationFrameResult RuntimeApplication::BeginFrame()
{
	if (!m_isInitialized)
	{
		return RuntimeApplicationFrameResult::Exit;
	}

	m_inputSystem->BeginFrame();
	m_window->PollEvents();
	m_inputSystem->ProcessDeferredEvents();

	if (m_window->ShouldClose())
	{
		return RuntimeApplicationFrameResult::Exit;
	}

	if (m_window->IsMinimized() || !m_window->HasValidSize())
	{
		m_window->WaitForEvent();
		return RuntimeApplicationFrameResult::SkipRender;
	}

	if (m_levelManager)
	{
		m_levelManager->ProcessPendingLevelChange();
	}

	return RuntimeApplicationFrameResult::Ready;
}

void RuntimeApplication::UpdateRuntime() noexcept
{
	if (m_gameWorld && m_timer)
	{
		m_cameraInputIntentCollector->Publish(*m_gameWorld);
		const float deltaSeconds = static_cast<float>(m_timer->GetDelta(TimeDomain::Scaled, TimeUnit::Seconds));
		m_gameWorld->Update(deltaSeconds);
		if (m_renderer && m_window)
		{
			RenderFrameMetadata metadata;
			metadata.FrameId = m_timer->GetFrameCount();
			metadata.ProviderGeneration = m_renderer->GetShaderPackageGeneration();
			metadata.RenderWidth = metadata.OutputWidth = m_window->GetWidth();
			metadata.RenderHeight = metadata.OutputHeight = m_window->GetHeight();
			m_renderer->SubmitRenderInput(m_gameWorld->ExtractRenderInput(metadata));
		}
	}
}

void RuntimeApplication::SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept
{
	if (m_renderer)
	{
		m_renderer->SubmitViewportRenderRequest(request);
	}
}

const ViewportRenderProducts& RuntimeApplication::GetViewportRenderProducts() const noexcept
{
	static const ViewportRenderProducts emptyProducts{};
	if (!m_renderer)
	{
		return emptyProducts;
	}

	return m_renderer->GetViewportRenderProducts();
}

bool RuntimeApplication::Tick()
{
	switch (BeginFrame())
	{
		case RuntimeApplicationFrameResult::Exit:
			return false;
		case RuntimeApplicationFrameResult::SkipRender:
			return true;
		case RuntimeApplicationFrameResult::Ready:
		default:
			break;
	}

	if (m_runtimeConsoleHost != nullptr)
	{
		m_runtimeConsoleHost->TickFrame(
		    *m_renderer,
		    [this]()
		    {
			    UpdateRuntime();
		    });
	}
	else
	{
		UpdateRuntime();
		m_renderer->OnRender();
	}
	return true;
}

void RuntimeApplication::Shutdown()
{
	if (!m_isInitialized)
	{
		return;
	}

	m_runtimeConsoleHost.reset();
	m_renderer.reset();
	m_levelManager.reset();
	m_cameraInputIntentCollector.reset();
	m_gameWorld.reset();
	m_taskRuntime.reset();
	m_inputSystem.reset();
	m_window.reset();
	m_timer.reset();
	m_isInitialized = false;
}
