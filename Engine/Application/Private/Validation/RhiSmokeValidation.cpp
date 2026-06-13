#include "PCH.h"

#include "Validation/RhiSmokeValidation.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Level/Level.h"
#include "Level/LevelManager.h"
#include "Platform/Public/Window/Window.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "Renderer.h"
#include "RuntimeApplication.h"

#include <algorithm>
#include <string>
#include <vector>

struct RhiSmokeValidationConfig
{
	bool Enabled = false;
	bool TraceLogging = false;
	std::uint32_t FrameLimit = 120;
	std::uint32_t RestoreFrame = 10;
	std::uint32_t MaximizeFrame = 20;
	std::uint32_t ShaderReloadFrame = 0;
	bool LevelSwitching = true;
	std::uint32_t LevelSwitchIntervalFrames = 15;
};

struct RhiSmokeValidationState
{
	std::uint32_t CompletedRenderFrames = 0;
	bool DiagnosticsLogged = false;
	bool RendererEvidenceLogged = false;
	bool EditorViewportEvidenceLogged = false;
	bool LevelSwitchingInitialized = false;
	bool LevelSwitchingFinished = false;
	bool Failed = false;
	std::uint32_t LastLevelSwitchFrame = 0;
	std::uint32_t CompletedLevelSwitches = 0;
	std::vector<std::string> LevelSwitchOrder;
	std::string PendingLevelName;
};

class RhiSmokeValidationRunner final
{
  public:
	static bool IsRequested() noexcept;
	static int RunProject() noexcept;

  private:
	static RhiSmokeValidationConfig LoadConfig() noexcept;
	static void ApplyLoggingConfig(const RhiSmokeValidationConfig& config) noexcept;
	static std::string GetActiveLevelName(const RuntimeApplication& app);
	static void LogDiagnosticsCapabilities(
	    const RhiSmokeValidationConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeValidationState& state) noexcept;
	static void LogRendererSmokeEvidence(
	    const RhiSmokeValidationConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeValidationState& state) noexcept;
	static void InitializeLevelSwitching(const RhiSmokeValidationConfig& config, RuntimeApplication& app, RhiSmokeValidationState& state) noexcept;
	static void AdvanceLevelSwitching(const RhiSmokeValidationConfig& config, RuntimeApplication& app, RhiSmokeValidationState& state) noexcept;
	static void Advance(const RhiSmokeValidationConfig& config, RuntimeApplication& app, RhiSmokeValidationState& state) noexcept;
	static bool TickRuntime(RuntimeApplication& app, const RhiSmokeValidationConfig& config, RhiSmokeValidationState& state) noexcept;
	static int RunProjectValidation(const RhiSmokeValidationConfig& config) noexcept;
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
	config.LevelSwitching = !Environment::GetFlag("SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING");
	config.LevelSwitchIntervalFrames = Environment::GetUInt32(
	    "SPARKLE_SMOKE_LEVEL_SWITCH_INTERVAL_FRAMES",
	    config.LevelSwitchIntervalFrames);
	return config;
}

void RhiSmokeValidationRunner::ApplyLoggingConfig(const RhiSmokeValidationConfig& config) noexcept
{
	if (config.Enabled && config.TraceLogging)
	{
		Logging::SetLevel(spdlog::level::trace);
	}
}

std::string RhiSmokeValidationRunner::GetActiveLevelName(const RuntimeApplication& app)
{
	const LevelManager* levelManager = app.GetLevelManager();
	if (levelManager == nullptr)
	{
		return {};
	}

	const LevelAsset* activeLevel = levelManager->GetActiveLevel();
	return activeLevel != nullptr ? std::string(activeLevel->GetName()) : std::string();
}

void RhiSmokeValidationRunner::LogDiagnosticsCapabilities(
    const RhiSmokeValidationConfig& config,
	RuntimeApplication& app,
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
	const RhiDiagnosticsCapabilities capabilities = renderHardware.GetDiagnosticsService().GetDiagnostics().GetCapabilities();
	
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
		    appLogger,
		    "RHI smoke validation: live object reporting is unavailable; inspect the active backend diagnostics log lines for the concrete environment or runtime reason.");
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

void RhiSmokeValidationRunner::LogRendererSmokeEvidence(
    const RhiSmokeValidationConfig& config,
    RuntimeApplication& app,
    RhiSmokeValidationState& state) noexcept
{
	if (!config.Enabled || state.RendererEvidenceLogged)
	{
		return;
	}

	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
	if (appLogger == nullptr)
	{
		return;
	}

	const RendererSmokeDiagnosticsSnapshot snapshot = app.GetRenderer().CaptureSmokeDiagnostics();
	SPDLOG_LOGGER_INFO(
	    appLogger,
	    "RHI smoke evidence: backend={} frameGraphUnresolvedBarrierWarnings={} upscalerProvider='{}' upscalerStatus={} "
	    "upscalerReason='{}' rayTracing={} inlineRayQuery={}",
	    RhiBackendApiToString(snapshot.BackendApi),
	    snapshot.FrameGraphUnresolvedBarrierWarnings,
	    snapshot.UpscalerProvider,
	    snapshot.UpscalerStatus,
	    snapshot.UpscalerReason,
	    snapshot.RayTracingSupported,
	    snapshot.InlineRayQuerySupported);

	if (snapshot.FrameGraphUnresolvedBarrierWarnings > 0)
	{
		state.Failed = true;
		SPDLOG_LOGGER_ERROR(
		    appLogger,
		    "RHI smoke validation: frame graph reported {} unresolved barrier warning(s).",
		    snapshot.FrameGraphUnresolvedBarrierWarnings);
	}

	state.RendererEvidenceLogged = true;
}

void RhiSmokeValidationRunner::InitializeLevelSwitching(
	const RhiSmokeValidationConfig& config,
	RuntimeApplication& app,
    RhiSmokeValidationState& state) noexcept
{
	if (!config.Enabled || !config.LevelSwitching || state.LevelSwitchingInitialized)
	{
		return;
	}

	state.LevelSwitchingInitialized = true;
	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
	LevelManager* levelManager = app.GetLevelManager();
	if (levelManager == nullptr)
	{
		state.Failed = true;
		SPDLOG_LOGGER_ERROR(appLogger, "RHI smoke validation: level switching requested but no LevelManager is available");
		return;
	}

	state.LevelSwitchOrder = levelManager->GetRegisteredLevelNames();
	const std::string activeLevelName = GetActiveLevelName(app);
	state.LevelSwitchOrder.erase(
	    std::remove(state.LevelSwitchOrder.begin(), state.LevelSwitchOrder.end(), activeLevelName),
	    state.LevelSwitchOrder.end());

	SPDLOG_LOGGER_INFO(
	    appLogger,
	    "RHI smoke validation: level switching initialized activeLevel='{}' switchTargets={}",
	    activeLevelName,
	    state.LevelSwitchOrder.size());

	if (state.LevelSwitchOrder.empty())
	{
		state.LevelSwitchingFinished = true;
	}
}

void RhiSmokeValidationRunner::AdvanceLevelSwitching(
	const RhiSmokeValidationConfig& config,
	RuntimeApplication& app,
    RhiSmokeValidationState& state) noexcept
{
	if (!config.Enabled || !config.LevelSwitching || state.LevelSwitchingFinished)
	{
		return;
	}

	InitializeLevelSwitching(config, app, state);

	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
	const std::string activeLevelName = GetActiveLevelName(app);
	if (!state.PendingLevelName.empty() && activeLevelName == state.PendingLevelName)
	{
		++state.CompletedLevelSwitches;
		SPDLOG_LOGGER_INFO(
		    appLogger,
		    "RHI smoke validation: completed level switch to '{}' ({}/{})",
		    activeLevelName,
		    state.CompletedLevelSwitches,
		    state.LevelSwitchOrder.size());
		state.PendingLevelName.clear();
		state.LastLevelSwitchFrame = state.CompletedRenderFrames;
	}

	if (state.PendingLevelName.empty() && state.CompletedLevelSwitches >= state.LevelSwitchOrder.size())
	{
		state.LevelSwitchingFinished = true;
		SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: completed all level switch targets");
		return;
	}

	if (!state.PendingLevelName.empty())
	{
		return;
	}

	const std::uint32_t interval = std::max<std::uint32_t>(config.LevelSwitchIntervalFrames, 1u);
	if (state.CompletedRenderFrames - state.LastLevelSwitchFrame < interval)
	{
		return;
	}

	const std::string& nextLevelName = state.LevelSwitchOrder[state.CompletedLevelSwitches];
	LevelManager* levelManager = app.GetLevelManager();
	if (levelManager == nullptr)
	{
		state.Failed = true;
		SPDLOG_LOGGER_ERROR(appLogger, "RHI smoke validation: lost LevelManager before requesting level switch to '{}'", nextLevelName);
		return;
	}

	state.PendingLevelName = nextLevelName;
	SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: requesting level switch to '{}'", nextLevelName);
	levelManager->RequestLevelChange(nextLevelName);
}

void RhiSmokeValidationRunner::Advance(
	const RhiSmokeValidationConfig& config,
	RuntimeApplication& app,
    RhiSmokeValidationState& state) noexcept
{
	if (!config.Enabled)
	{
		return;
	}

	Window& window = app.GetWindow();
	++state.CompletedRenderFrames;
	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
	AdvanceLevelSwitching(config, app, state);

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
		if (config.LevelSwitching && !state.LevelSwitchingFinished)
		{
			state.Failed = true;
			SPDLOG_LOGGER_ERROR(
			    appLogger,
			    "RHI smoke validation: frame limit {} reached before level switching completed ({}/{})",
			    config.FrameLimit,
			    state.CompletedLevelSwitches,
			    state.LevelSwitchOrder.size());
		}

		if (appLogger != nullptr)
		{
			SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: reached frame limit {}, requesting shutdown", config.FrameLimit);
		}
		window.RequestClose();
	}
}

bool RhiSmokeValidationRunner::TickRuntime(
	RuntimeApplication& app,
	const RhiSmokeValidationConfig& config,
	RhiSmokeValidationState& state) noexcept
{
	switch (app.BeginFrame())
	{
		case RuntimeApplicationFrameResult::Exit:
			return false;
		case RuntimeApplicationFrameResult::SkipRender:
			return true;
		case RuntimeApplicationFrameResult::Ready:
		default:
			break;
	}

	app.UpdateRuntime();
	app.GetRenderer().OnRender();
	LogRendererSmokeEvidence(config, app, state);
	app.EndFrame();
	Advance(config, app, state);
	return true;
}

int RhiSmokeValidationRunner::RunProjectValidation(const RhiSmokeValidationConfig& config) noexcept
{
	RuntimeApplication app;
	RhiSmokeValidationState state{};
	ApplyLoggingConfig(config);
	app.Initialize();
	LogDiagnosticsCapabilities(config, app, state);
	InitializeLevelSwitching(config, app, state);

	while (TickRuntime(app, config, state))
	{
	}

	app.Shutdown();
	return state.Failed ? 1 : 0;
}

bool RhiSmokeValidationRunner::IsRequested() noexcept
{
	return LoadConfig().Enabled;
}

int RhiSmokeValidationRunner::RunProject() noexcept
{
	return RunProjectValidation(LoadConfig());
}

bool RhiSmokeValidation::IsRequested() noexcept
{
	return RhiSmokeValidationRunner::IsRequested();
}

int RhiSmokeValidation::RunProject() noexcept
{
	return RhiSmokeValidationRunner::RunProject();
}
