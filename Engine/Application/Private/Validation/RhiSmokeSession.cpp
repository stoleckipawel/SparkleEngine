#include "PCH.h"

#include "Validation/RhiSmokeSession.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "RuntimeApplication.h"
#include "Validation/RhiSmokeRendererEvidence.h"

namespace RhiSmokeSession
{
	RhiSmokeSessionConfig LoadConfig() noexcept
	{
		RhiSmokeSessionConfig config{};
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

	void ApplyLoggingConfig(const RhiSmokeSessionConfig& config) noexcept
	{
		if (config.Enabled && config.TraceLogging)
		{
			Logging::SetLevel(spdlog::level::trace);
		}
	}

	void LogDiagnosticsCapabilities(const RhiSmokeSessionConfig& config, RuntimeApplication& app, RhiSmokeSessionState& state) noexcept
	{
		if (config.Enabled)
		{
			RhiSmokeRendererEvidence::LogRhiDiagnosticsCapabilities(app, state.DiagnosticsLogged);
		}
	}

	void LogRendererEvidence(
	    const RhiSmokeSessionConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeSessionState& state,
	    std::string_view evidenceLabel,
	    std::string_view validationLabel) noexcept
	{
		if (!config.Enabled)
		{
			return;
		}

		if (!RhiSmokeRendererEvidence::LogRendererEvidence(app, state.RendererEvidenceLogged, evidenceLabel, validationLabel))
		{
			state.FrameControl.Failed = true;
		}
	}

	void InitializeFrameControl(const RhiSmokeSessionConfig& config, RuntimeApplication& app, RhiSmokeSessionState& state) noexcept
	{
		RhiSmokeFrameControl::InitializeLevelSwitching(config.FrameControl, app, state.FrameControl);
	}
}
