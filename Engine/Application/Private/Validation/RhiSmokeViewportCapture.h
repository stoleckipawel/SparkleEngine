#pragma once

#include "Renderer/Public/Debug/RendererCVars.h"

#include <cstdint>
#include <filesystem>
#include <string>

class RuntimeApplication;
struct ViewportPresentationProduct;
struct ViewportRenderProducts;

struct RhiSmokeViewportCaptureConfig final
{
	std::uint32_t SceneColorCaptureFrame = 20;
	std::string SceneColorCapturePath;
	std::string MetadataPath;
	std::string TimingCsvPath;
	std::string CapturePurpose;
	std::string CaptureLabel;
	bool HasViewModeOverride = false;
	RenderViewMode ViewModeOverride = RenderViewMode::Lit;
};

struct RhiSmokeViewportCaptureState final
{
	bool ViewportEvidenceLogged = false;
	bool SceneColorCaptured = false;
	bool ViewModeOverrideLogged = false;
};

namespace RhiSmokeViewportCapture
{
	void ApplyViewModeOverride(const RhiSmokeViewportCaptureConfig& config, RhiSmokeViewportCaptureState& state) noexcept;
	void LogEvidence(
	    bool enabled,
	    const ViewportRenderProducts& viewportProducts,
	    const ViewportPresentationProduct& sceneColorPresentation,
	    RhiSmokeViewportCaptureState& state) noexcept;
	void CaptureSceneColorIfRequested(
	    bool enabled,
	    const RhiSmokeViewportCaptureConfig& config,
	    RuntimeApplication& app,
	    std::uint32_t completedRenderFrames,
	    RhiSmokeViewportCaptureState& state,
	    bool& failed) noexcept;
}
