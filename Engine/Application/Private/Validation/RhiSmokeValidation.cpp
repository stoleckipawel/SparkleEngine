#include "PCH.h"

#include "Validation/RhiSmokeValidation.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Diagnostics/ScopedLogEvent.h"
#include "Editor/Public/UI.h"
#include "Input/InputSystem.h"
#include "Platform/Public/Window/Window.h"
#include "ProjectApp.h"
#include "Renderer.h"

struct RhiSmokeValidationConfig
{
	bool Enabled = false;
	bool TraceLogging = false;
	std::uint32_t FrameLimit = 120;
	std::uint32_t RestoreFrame = 10;
	std::uint32_t MaximizeFrame = 20;
	std::uint32_t ShaderReloadFrame = 0;
};

struct RhiSmokeValidationState
{
	std::uint32_t CompletedRenderFrames = 0;
	bool DiagnosticsLogged = false;
	bool EditorViewportEvidenceLogged = false;
};

class RhiSmokeValidationRunner final
{
  public:
	static bool IsRequested() noexcept;
	static int RunProject() noexcept;
	static int RunEditor() noexcept;

  private:
	static RhiSmokeValidationConfig LoadConfig() noexcept;
	static void ApplyLoggingConfig(const RhiSmokeValidationConfig& config) noexcept;
	static void LogDiagnosticsCapabilities(
	    const RhiSmokeValidationConfig& config,
	    ProjectApp& app,
	    RhiSmokeValidationState& state) noexcept;
	static void LogEditorViewportEvidence(
	    const RhiSmokeValidationConfig& config,
	    ProjectApp& app,
	    const ViewportRenderProducts& viewportProducts,
	    RhiSmokeValidationState& state) noexcept;
	static void Advance(const RhiSmokeValidationConfig& config, ProjectApp& app, RhiSmokeValidationState& state) noexcept;
	static bool TickRuntime(ProjectApp& app, const RhiSmokeValidationConfig& config, RhiSmokeValidationState& state) noexcept;
	static bool TickEditor(ProjectApp& app, UI& ui, const RhiSmokeValidationConfig& config, RhiSmokeValidationState& state) noexcept;
	static int RunProjectValidation(const RhiSmokeValidationConfig& config) noexcept;
	static int RunEditorValidation(const RhiSmokeValidationConfig& config) noexcept;
};

RhiSmokeValidationConfig RhiSmokeValidationRunner::LoadConfig() noexcept
{
	RhiSmokeValidationConfig config{};
	config.Enabled = Environment::GetFlag("SPARKLE_SMOKE_VALIDATE_RHI");
	if (!config.Enabled)
	{
		return config;
	}

	config.TraceLogging = Environment::GetFlag("SPARKLE_SMOKE_TRACE");
	config.FrameLimit = Environment::GetUInt32("SPARKLE_SMOKE_FRAME_LIMIT", config.FrameLimit);
	config.RestoreFrame = Environment::GetUInt32("SPARKLE_SMOKE_RESTORE_FRAME", config.RestoreFrame);
	config.MaximizeFrame = Environment::GetUInt32("SPARKLE_SMOKE_MAXIMIZE_FRAME", config.MaximizeFrame);
	config.ShaderReloadFrame = Environment::GetUInt32("SPARKLE_SMOKE_SHADER_RELOAD_FRAME", config.ShaderReloadFrame);
	return config;
}

void RhiSmokeValidationRunner::ApplyLoggingConfig(const RhiSmokeValidationConfig& config) noexcept
{
	if (config.Enabled && config.TraceLogging)
	{
		Logging::SetLevel(spdlog::level::trace);
	}
}

void RhiSmokeValidationRunner::LogDiagnosticsCapabilities(
    const RhiSmokeValidationConfig& config,
    ProjectApp& app,
    RhiSmokeValidationState& state) noexcept
{
	if (!config.Enabled || state.DiagnosticsLogged)
	{
		return;
	}

	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
	if (appLogger == nullptr)
	{
		return;
	}

	const RenderHardwareInterface& renderHardware = app.GetRenderer().GetRenderHardwareInterface();
	const RhiDiagnosticsCapabilities capabilities = renderHardware.GetDiagnostics().GetCapabilities();
	SPDLOG_LOGGER_INFO(
	    appLogger,
	    "RHI smoke diagnostics capabilities: objectNames={} commandScopes={} timestampQueries={} debugMessages={} liveObjectReports={} "
	    "crashDiagnostics={}",
	    capabilities.SupportsObjectNames,
	    capabilities.SupportsGpuEvents,
	    capabilities.SupportsTimestampQueries,
	    capabilities.SupportsDebugMessages,
	    capabilities.SupportsLiveObjectReports,
	    capabilities.SupportsCrashDiagnostics);

	if (!capabilities.SupportsGpuEvents)
	{
		SPDLOG_LOGGER_WARN(
		    appLogger,
		    "RHI smoke validation: command scopes are unavailable because the active backend could not initialize GPU event markers.");
	}
	if (!capabilities.SupportsTimestampQueries)
	{
		SPDLOG_LOGGER_WARN(appLogger, "RHI smoke validation: timestamp queries are unavailable on the current backend/device path.");
	}
	if (!capabilities.SupportsDebugMessages)
	{
		SPDLOG_LOGGER_WARN(
		    appLogger,
		    "RHI smoke validation: debug messages are unavailable; inspect the active backend diagnostics log lines for the concrete "
		    "environment or runtime reason.");
	}
	if (!capabilities.SupportsLiveObjectReports)
	{
		SPDLOG_LOGGER_WARN(
		    appLogger, "RHI smoke validation: live object reporting is unavailable; inspect the active backend diagnostics log lines for the concrete environment or runtime reason.");
	}
	if (!capabilities.SupportsCrashDiagnostics)
	{
		SPDLOG_LOGGER_WARN(
		    appLogger,
		    "RHI smoke validation: crash diagnostics are unavailable; inspect the active backend diagnostics log lines for the concrete "
		    "environment or runtime reason.");
	}

	state.DiagnosticsLogged = true;
}

void RhiSmokeValidationRunner::LogEditorViewportEvidence(
	const RhiSmokeValidationConfig& config,
    ProjectApp& app,
    const ViewportRenderProducts& viewportProducts,
	RhiSmokeValidationState& state) noexcept
{
	if (!config.Enabled || state.EditorViewportEvidenceLogged)
	{
		return;
	}

	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
	if (appLogger == nullptr)
	{
		return;
	}

	Renderer& renderer = app.GetRenderer();
	const std::uint64_t sceneColorTextureId = renderer.ResolveRenderProductTextureId(viewportProducts.SceneColor.Handle);
	SPDLOG_LOGGER_INFO(
	    appLogger,
	    "RHI editor smoke evidence: viewport sceneColorHandle={} textureId={} extent={}x{} outputsMask={}",
	    viewportProducts.SceneColor.Handle.Value,
	    sceneColorTextureId,
	    viewportProducts.SceneColor.Extent.Width,
	    viewportProducts.SceneColor.Extent.Height,
	    static_cast<std::uint32_t>(viewportProducts.AvailableOutputs));

	state.EditorViewportEvidenceLogged = true;
}

void RhiSmokeValidationRunner::Advance(
	const RhiSmokeValidationConfig& config,
    ProjectApp& app,
	RhiSmokeValidationState& state) noexcept
{
	if (!config.Enabled)
	{
		return;
	}

	Window& window = app.GetWindow();
	++state.CompletedRenderFrames;
	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");

	if (config.ShaderReloadFrame > 0 && state.CompletedRenderFrames == config.ShaderReloadFrame)
	{
		Renderer& renderer = app.GetRenderer();
		renderer.GetRenderHardwareInterface().WaitForIdle();
		const CookedShaderReloadResult reloadResult = renderer.ReloadCookedShaders();
		if (appLogger != nullptr)
		{
			if (reloadResult)
			{
				SPDLOG_LOGGER_INFO(
				    appLogger,
				    "RHI smoke validation: reloaded cooked shaders on frame {} (generation={})",
				    state.CompletedRenderFrames,
				    renderer.GetShaderPackageGeneration());
			}
			else
			{
				SPDLOG_LOGGER_ERROR(
				    appLogger,
				    "RHI smoke validation: cooked shader reload was rejected on frame {}. {}",
				    state.CompletedRenderFrames,
				    reloadResult.ErrorMessage);
			}
		}
	}

	if (config.RestoreFrame > 0 && state.CompletedRenderFrames == config.RestoreFrame)
	{
		if (appLogger != nullptr)
		{
			SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: restoring window on frame {}", state.CompletedRenderFrames);
		}
		window.Restore();
	}

	if (config.MaximizeFrame > 0 && state.CompletedRenderFrames == config.MaximizeFrame)
	{
		if (appLogger != nullptr)
		{
			SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: maximizing window on frame {}", state.CompletedRenderFrames);
		}
		window.Maximize();
	}

	if (config.FrameLimit > 0 && state.CompletedRenderFrames >= config.FrameLimit)
	{
		if (appLogger != nullptr)
		{
			SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: reached frame limit {}, requesting shutdown", config.FrameLimit);
		}
		window.RequestClose();
	}
}

bool RhiSmokeValidationRunner::TickRuntime(
    ProjectApp& app,
	const RhiSmokeValidationConfig& config,
	RhiSmokeValidationState& state) noexcept
{
	switch (app.BeginFrame())
	{
		case ProjectAppFrameResult::Exit:
			return false;
		case ProjectAppFrameResult::SkipRender:
			return true;
		case ProjectAppFrameResult::Ready:
		default:
			break;
	}

	app.UpdateRuntime();
	app.GetRenderer().OnRender();
	app.EndFrame();
	Advance(config, app, state);
	return true;
}

bool RhiSmokeValidationRunner::TickEditor(
    ProjectApp& app,
    UI& ui,
	const RhiSmokeValidationConfig& config,
	RhiSmokeValidationState& state) noexcept
{
	switch (app.BeginFrame())
	{
		case ProjectAppFrameResult::Exit:
			return false;
		case ProjectAppFrameResult::SkipRender:
			return true;
		case ProjectAppFrameResult::Ready:
		default:
			break;
	}

	app.UpdateRuntime();
	app.SubmitViewportRenderRequest(ui.GetViewportRenderRequest());

	Renderer& renderer = app.GetRenderer();
	renderer.PrepareHostFrame();
	renderer.RecordHostFrame();

	const ViewportRenderProducts& viewportProducts = app.GetViewportRenderProducts();
	ui.SetViewportRenderProducts(viewportProducts);
	ui.SetViewportSceneColorTextureId(renderer.ResolveRenderProductTextureId(viewportProducts.SceneColor.Handle));
	LogEditorViewportEvidence(config, app, viewportProducts, state);
	ui.Update();

	RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();
	const NativeGraphicsCommandListHandle commandListHandle =
	    renderHardware.GetGraphicsCommandListHandle(renderHardware.GetCurrentFrameIndex());

	renderer.TransitionRenderProduct(
	    commandListHandle,
	    viewportProducts.SceneColor.Handle,
	    ResourceState::RenderTarget,
	    ResourceState::ShaderResource);

	constexpr float editorClearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
	renderHardware.BeginPresentRenderPass(commandListHandle, editorClearColor);
	ui.Render(commandListHandle);
	renderHardware.EndPresentRenderPass(commandListHandle);

	renderer.TransitionRenderProduct(
	    commandListHandle,
	    viewportProducts.SceneColor.Handle,
	    ResourceState::ShaderResource,
	    ResourceState::Common);

	renderer.SubmitHostFrame();
	app.EndFrame();
	Advance(config, app, state);
	return true;
}

int RhiSmokeValidationRunner::RunProjectValidation(const RhiSmokeValidationConfig& config) noexcept
{
	ProjectApp app;
	RhiSmokeValidationState state{};
	ApplyLoggingConfig(config);
	app.Initialize();
	LogDiagnosticsCapabilities(config, app, state);

	while (TickRuntime(app, config, state))
	{
	}

	app.Shutdown();
	return 0;
}

int RhiSmokeValidationRunner::RunEditorValidation(const RhiSmokeValidationConfig& config) noexcept
{
	ProjectApp app;
	RhiSmokeValidationState state{};
	ApplyLoggingConfig(config);
	app.Initialize();
	LogDiagnosticsCapabilities(config, app, state);
	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");

	{
		SPARKLE_LOG_SCOPE(appLogger, spdlog::level::info, "RHI editor smoke UI scope");
		Renderer& renderer = app.GetRenderer();
		RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();
		app.GetInputSystem().SetAutomaticImGuiCaptureEnabled(false);
		app.GetInputSystem().BeginInputRoutingFrame(false, false);
		UI ui(app.GetTimer(), app.GetLevelManager(), app.GetGameScene(), renderHardware, app.GetWindow(), app.GetInputSystem());

		while (TickEditor(app, ui, config, state))
		{
		}
		app.GetInputSystem().SetAutomaticImGuiCaptureEnabled(true);
	}

	app.Shutdown();
	SPDLOG_LOGGER_INFO(appLogger, "RHI editor smoke: ProjectApp shutdown complete");
	return 0;
}

bool RhiSmokeValidationRunner::IsRequested() noexcept
{
	return LoadConfig().Enabled;
}

int RhiSmokeValidationRunner::RunProject() noexcept
{
	return RunProjectValidation(LoadConfig());
}

int RhiSmokeValidationRunner::RunEditor() noexcept
{
	return RunEditorValidation(LoadConfig());
}

bool RhiSmokeValidation::IsRequested() noexcept
{
	return RhiSmokeValidationRunner::IsRequested();
}

int RhiSmokeValidation::RunProject() noexcept
{
	return RhiSmokeValidationRunner::RunProject();
}

int RhiSmokeValidation::RunEditor() noexcept
{
	return RhiSmokeValidationRunner::RunEditor();
}