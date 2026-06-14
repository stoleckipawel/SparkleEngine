#include "PCH.h"

#include "Validation/RhiSmokeValidation.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Renderer.h"
#include "Validation/RhiSmokeFrameControl.h"
#include "Validation/RhiSmokeRendererEvidence.h"
#include "RuntimeApplication.h"

struct RhiSmokeValidationConfig
{
	bool Enabled = false;
	bool TraceLogging = false;
	RhiSmokeFrameControlConfig FrameControl;
};

struct RhiSmokeValidationState
{
	bool DiagnosticsLogged = false;
	bool RendererEvidenceLogged = false;
	RhiSmokeFrameControlState FrameControl;
};

class RhiSmokeValidationRunner final
{
  public:
	static bool IsRequested() noexcept;
	static int RunProject() noexcept;

  private:
	static RhiSmokeValidationConfig LoadConfig() noexcept;
	static void ApplyLoggingConfig(const RhiSmokeValidationConfig& config) noexcept;
	static void LogDiagnosticsCapabilities(
	    const RhiSmokeValidationConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeValidationState& state) noexcept;
	static void LogRendererSmokeEvidence(
	    const RhiSmokeValidationConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeValidationState& state) noexcept;
	static void InitializeFrameControl(
	    const RhiSmokeValidationConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeValidationState& state) noexcept;
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
	config.FrameControl.Enabled = config.Enabled;
	config.FrameControl.FrameLimit = Environment::GetUInt32("SPARKLE_SMOKE_FRAME_LIMIT", config.FrameControl.FrameLimit);
	config.FrameControl.RestoreFrame = Environment::GetUInt32("SPARKLE_SMOKE_RESTORE_FRAME", config.FrameControl.RestoreFrame);
	config.FrameControl.MaximizeFrame = Environment::GetUInt32("SPARKLE_SMOKE_MAXIMIZE_FRAME", config.FrameControl.MaximizeFrame);
	config.FrameControl.ShaderReloadFrame =
	    Environment::GetUInt32("SPARKLE_SMOKE_SHADER_RELOAD_FRAME", config.FrameControl.ShaderReloadFrame);
	config.FrameControl.LevelSwitching = !Environment::GetFlag("SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING");
	config.FrameControl.LevelSwitchIntervalFrames =
	    Environment::GetUInt32("SPARKLE_SMOKE_LEVEL_SWITCH_INTERVAL_FRAMES", config.FrameControl.LevelSwitchIntervalFrames);
	config.FrameControl.CameraMotion = RhiSmokeCameraMotion::LoadConfig();
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
    RuntimeApplication& app,
    RhiSmokeValidationState& state) noexcept
{
	if (config.Enabled)
	{
		RhiSmokeRendererEvidence::LogRhiDiagnosticsCapabilities(app, state.DiagnosticsLogged);
	}
}

void RhiSmokeValidationRunner::LogRendererSmokeEvidence(
    const RhiSmokeValidationConfig& config,
    RuntimeApplication& app,
    RhiSmokeValidationState& state) noexcept
{
	if (!config.Enabled)
	{
		return;
	}

	if (!RhiSmokeRendererEvidence::LogRendererEvidence(app, state.RendererEvidenceLogged, "RHI smoke evidence", "RHI smoke validation"))
	{
		state.FrameControl.Failed = true;
	}
}

void RhiSmokeValidationRunner::InitializeFrameControl(
    const RhiSmokeValidationConfig& config,
    RuntimeApplication& app,
    RhiSmokeValidationState& state) noexcept
{
	RhiSmokeFrameControl::InitializeLevelSwitching(config.FrameControl, app, state.FrameControl);
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
	RhiSmokeFrameControl::Advance(config.FrameControl, app, state.FrameControl, "runtime");
	return true;
}

int RhiSmokeValidationRunner::RunProjectValidation(const RhiSmokeValidationConfig& config) noexcept
{
	RuntimeApplication app;
	RhiSmokeValidationState state{};
	ApplyLoggingConfig(config);
	app.Initialize();
	LogDiagnosticsCapabilities(config, app, state);
	InitializeFrameControl(config, app, state);

	while (TickRuntime(app, config, state))
	{
	}

	app.Shutdown();
	return state.FrameControl.Failed ? 1 : 0;
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
