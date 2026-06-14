#include "PCH.h"

#include "Validation/RhiSmokeEditorViewport.h"

#include "Renderer.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RuntimeApplication.h"

#include <algorithm>

namespace
{
	std::shared_ptr<spdlog::logger> GetSmokeLogger()
	{
		return Logging::GetOrCreateLogger("Application.SmokeValidation");
	}

	const char* RenderViewModeName(RenderViewMode viewMode) noexcept
	{
		switch (viewMode)
		{
			case RenderViewMode::Lit:
				return "Lit";
			case RenderViewMode::Wireframe:
				return "Wireframe";
			case RenderViewMode::GBufferDiffuse:
				return "GBufferDiffuse";
			case RenderViewMode::GBufferNormal:
				return "GBufferNormal";
			case RenderViewMode::GBufferRoughness:
				return "GBufferRoughness";
			case RenderViewMode::GBufferMetallic:
				return "GBufferMetallic";
			case RenderViewMode::GBufferEmissive:
				return "GBufferEmissive";
			case RenderViewMode::GBufferAmbientOcclusion:
				return "GBufferAmbientOcclusion";
			case RenderViewMode::GBufferSubsurfaceColor:
				return "GBufferSubsurfaceColor";
			case RenderViewMode::GBufferSubsurfaceStrength:
				return "GBufferSubsurfaceStrength";
			case RenderViewMode::DirectDiffuse:
				return "DirectDiffuse";
			case RenderViewMode::DirectSpecular:
				return "DirectSpecular";
			case RenderViewMode::DirectSubsurface:
				return "DirectSubsurface";
			case RenderViewMode::IndirectDiffuse:
				return "IndirectDiffuse";
			case RenderViewMode::IndirectSpecular:
				return "IndirectSpecular";
			case RenderViewMode::IndirectSubsurface:
				return "IndirectSubsurface";
			case RenderViewMode::InstanceGroups:
				return "InstanceGroups";
			case RenderViewMode::Count:
				break;
		}

		return "Unknown";
	}
}

namespace RhiSmokeEditorViewport
{
	void ApplyViewModeOverride(const RhiSmokeEditorViewportConfig& config, RhiSmokeEditorViewportState& state) noexcept
	{
		if (!config.HasViewModeOverride)
		{
			return;
		}

		CVarRenderViewMode.Set(config.ViewModeOverride);
		if (!state.ViewModeOverrideLogged)
		{
			const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
			SPDLOG_LOGGER_INFO(logger, "RHI editor smoke: forced render view mode {}", static_cast<std::uint32_t>(config.ViewModeOverride));
			state.ViewModeOverrideLogged = true;
		}
	}

	void LogEvidence(
	    bool enabled,
	    const ViewportRenderProducts& viewportProducts,
	    const ViewportPresentationProduct& sceneColorPresentation,
	    RhiSmokeEditorViewportState& state) noexcept
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
		    "RHI editor smoke evidence: viewport sceneColorHandle={} textureId={} extent={}x{} outputsMask={} presentationStatus={} "
		    "reason='{}'",
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
	    const RhiSmokeEditorViewportConfig& config,
	    RuntimeApplication& app,
	    std::uint32_t completedRenderFrames,
	    RhiSmokeEditorViewportState& state,
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
		        .ViewModeName = RenderViewModeName(viewMode),
		        .DebugName = "Editor smoke scene color"});

		if (captureResult)
		{
			SPDLOG_LOGGER_INFO(
			    logger,
			    "RHI editor smoke: captured scene color backend={} viewMode={}({}) path='{}' frame={}",
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
			    "RHI editor smoke: failed to capture scene color backend={} viewMode={}({}) path='{}' frame={} reason='{}'",
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
