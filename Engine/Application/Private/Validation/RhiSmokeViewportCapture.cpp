#include "PCH.h"

#include "Validation/RhiSmokeViewportCapture.h"

#include "Renderer.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RuntimeApplication.h"
#include "Validation/RhiSmokeCaptureArtifacts.h"
#include "Validation/RhiSmokeRenderViewModeNames.h"

#include <algorithm>

namespace
{
	std::shared_ptr<spdlog::logger> GetSmokeLogger()
	{
		return Logging::GetOrCreateLogger("Application.SmokeValidation");
	}
}

namespace RhiSmokeViewportCapture
{
	void ApplyViewModeOverride(const RhiSmokeViewportCaptureConfig& config, RhiSmokeViewportCaptureState& state) noexcept
	{
		if (!config.HasViewModeOverride)
		{
			return;
		}

		CVarRenderViewMode.Set(config.ViewModeOverride);
		if (!state.ViewModeOverrideLogged)
		{
			const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
			SPDLOG_LOGGER_INFO(
			    logger,
			    "RHI smoke: forced render view mode {}({})",
			    RhiSmokeRenderViewModeNames::ToString(config.ViewModeOverride),
			    static_cast<std::uint32_t>(config.ViewModeOverride));
			state.ViewModeOverrideLogged = true;
		}
	}

	void LogEvidence(
	    bool enabled,
	    const ViewportRenderProducts& viewportProducts,
	    const ViewportPresentationProduct& sceneColorPresentation,
	    RhiSmokeViewportCaptureState& state) noexcept
	{
		if (!enabled || state.ViewportEvidenceLogged)
		{
			return;
		}

		const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
		if (logger == nullptr)
		{
			return;
		}

		const RenderProduct& sceneColor = viewportProducts.GetSceneColor();
		SPDLOG_LOGGER_INFO(
		    logger,
		    "RHI smoke viewport evidence: sceneColorHandle={} textureId={} extent={}x{} outputsMask={} presentationStatus={} reason='{}'",
		    sceneColor.Handle.Value,
		    sceneColorPresentation.TextureId,
		    sceneColor.Extent.Width,
		    sceneColor.Extent.Height,
		    static_cast<std::uint32_t>(viewportProducts.GetAvailableOutputs()),
		    static_cast<std::uint32_t>(sceneColorPresentation.Status),
		    sceneColorPresentation.FailureReason);

		state.ViewportEvidenceLogged = true;
	}

	void CaptureSceneColorIfRequested(
	    bool enabled,
	    const RhiSmokeViewportCaptureConfig& config,
	    RuntimeApplication& app,
	    std::uint32_t completedRenderFrames,
	    RhiSmokeViewportCaptureState& state,
	    bool& failed) noexcept
	{
		if (!enabled || state.SceneColorCaptured || config.SceneColorCapturePath.empty())
		{
			return;
		}

		const std::uint32_t currentFrame = completedRenderFrames + 1u;
		if (currentFrame < std::max<std::uint32_t>(config.SceneColorCaptureFrame, 1u))
		{
			return;
		}

		const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
		Renderer& renderer = app.GetRenderer();
		const RenderViewMode viewMode = config.HasViewModeOverride ? config.ViewModeOverride : CVarRenderViewMode.Get();
		const RhiCaptureResult captureResult = renderer.CaptureViewportProductToBmp(
		    ViewportCaptureRequest{
		        .Output = RenderOutputFlags::SceneColor,
		        .OutputPath = std::filesystem::path(config.SceneColorCapturePath),
		        .FrameIndex = currentFrame,
		        .ViewMode = static_cast<std::uint32_t>(viewMode),
		        .ViewModeName = RhiSmokeRenderViewModeNames::ToString(viewMode),
		        .DebugName = "Smoke scene color"});
		RhiSmokeCaptureArtifacts::Write(
		    RhiSmokeCaptureArtifactRequest{
		        .CaptureResult = captureResult,
		        .Diagnostics = renderer.CaptureSmokeDiagnostics(),
		        .CapturePath = std::filesystem::path(config.SceneColorCapturePath),
		        .MetadataPath = std::filesystem::path(config.MetadataPath),
		        .TimingCsvPath = std::filesystem::path(config.TimingCsvPath),
		        .CapturePurpose = config.CapturePurpose,
		        .CaptureLabel = config.CaptureLabel});

		if (captureResult)
		{
			SPDLOG_LOGGER_INFO(
			    logger,
			    "RHI smoke: captured scene color backend={} viewMode={}({}) path='{}' frame={}",
			    RhiBackendApiToString(captureResult.BackendApi),
			    captureResult.ViewModeName,
			    captureResult.ViewMode,
			    captureResult.ArtifactPath.string(),
			    captureResult.FrameIndex);
			state.SceneColorCaptured = true;
		}
		else
		{
			SPDLOG_LOGGER_ERROR(
			    logger,
			    "RHI smoke: failed to capture scene color backend={} viewMode={}({}) path='{}' frame={} reason='{}'",
			    RhiBackendApiToString(captureResult.BackendApi),
			    captureResult.ViewModeName,
			    captureResult.ViewMode,
			    config.SceneColorCapturePath,
			    captureResult.FrameIndex,
			    captureResult.FailureReason);
			failed = true;
			state.SceneColorCaptured = true;
		}
	}
}
