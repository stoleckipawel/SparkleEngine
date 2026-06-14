#include "PCH.h"

#include "Validation/RhiSmokeFrameControl.h"

#include "Platform/Public/Window/Window.h"
#include "Renderer.h"
#include "RuntimeApplication.h"

namespace
{
	std::shared_ptr<spdlog::logger> GetSmokeLogger()
	{
		return Logging::GetOrCreateLogger("Application.SmokeValidation");
	}
}

namespace RhiSmokeFrameControl
{
	void InitializeLevelSwitching(
	    const RhiSmokeFrameControlConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeFrameControlState& state) noexcept
	{
		RhiSmokeLevelSwitching::Initialize(
		    RhiSmokeLevelSwitchingConfig{
		        .Enabled = config.Enabled && config.LevelSwitching,
		        .IntervalFrames = config.LevelSwitchIntervalFrames},
		    app,
		    state.LevelSwitching,
		    state.Failed);
	}

	void Advance(
	    const RhiSmokeFrameControlConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeFrameControlState& state,
	    const char* validationContext) noexcept
	{
		if (!config.Enabled)
		{
			return;
		}

		Window& window = app.GetWindow();
		++state.CompletedRenderFrames;
		const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
		RhiSmokeLevelSwitching::Advance(
		    RhiSmokeLevelSwitchingConfig{
		        .Enabled = config.Enabled && config.LevelSwitching,
		        .IntervalFrames = config.LevelSwitchIntervalFrames},
		    app,
		    state.CompletedRenderFrames,
		    state.LevelSwitching,
		    state.Failed);
		RhiSmokeCameraMotion::Advance(config.CameraMotion, app, state.CompletedRenderFrames, state.CameraMotion);

		if (config.ShaderReloadFrame > 0 && state.CompletedRenderFrames == config.ShaderReloadFrame)
		{
			Renderer& renderer = app.GetRenderer();
			renderer.GetCommandSubmissionService().WaitForIdle();
			const CookedShaderReloadResult reloadResult = renderer.ReloadCookedShaders();
			if (reloadResult)
			{
				SPDLOG_LOGGER_INFO(
				    logger,
				    "RHI smoke validation: reloaded cooked shaders on frame {} (generation={})",
				    state.CompletedRenderFrames,
				    renderer.GetShaderPackageGeneration());
			}
			else
			{
				state.Failed = true;
				SPDLOG_LOGGER_ERROR(
				    logger,
				    "RHI smoke validation: cooked shader reload was rejected on frame {}. {}",
				    state.CompletedRenderFrames,
				    reloadResult.ErrorMessage);
			}
		}

		if (config.RestoreFrame > 0 && state.CompletedRenderFrames == config.RestoreFrame)
		{
			SPDLOG_LOGGER_INFO(logger, "RHI smoke validation: restoring window on frame {}", state.CompletedRenderFrames);
			window.Restore();
		}

		if (config.MaximizeFrame > 0 && state.CompletedRenderFrames == config.MaximizeFrame)
		{
			SPDLOG_LOGGER_INFO(logger, "RHI smoke validation: maximizing window on frame {}", state.CompletedRenderFrames);
			window.Maximize();
		}

		if (config.FrameLimit > 0 && state.CompletedRenderFrames >= config.FrameLimit)
		{
			if (!RhiSmokeCameraMotion::Validate(
			        config.CameraMotion,
			        state.CameraMotion,
			        app.GetRenderer().CaptureSmokeDiagnostics(),
			        validationContext))
			{
				state.Failed = true;
				SPDLOG_LOGGER_ERROR(logger, "RHI smoke validation: camera motion evidence failed validation");
			}

			if (config.LevelSwitching && !state.LevelSwitching.Finished)
			{
				state.Failed = true;
				SPDLOG_LOGGER_ERROR(
				    logger,
				    "RHI smoke validation: frame limit {} reached before level switching completed ({}/{})",
				    config.FrameLimit,
				    state.LevelSwitching.CompletedSwitches,
				    state.LevelSwitching.SwitchOrder.size());
			}

			SPDLOG_LOGGER_INFO(logger, "RHI smoke validation: reached frame limit {}, requesting shutdown", config.FrameLimit);
			window.RequestClose();
		}
	}
}
