#pragma once

#include "Renderer/Public/Debug/RendererCVars.h"

#include <cstdint>
#include <filesystem>
#include <string>

class RuntimeApplication;
struct ViewportPresentationProduct;
struct ViewportRenderProducts;

struct RhiSmokeEditorViewportConfig final
{
	std::uint32_t SceneColorCaptureFrame = 20;
	std::string SceneColorCapturePath;
	std::string MetadataPath;
	std::string TimingCsvPath;
	bool HasViewModeOverride = false;
	RenderViewMode ViewModeOverride = RenderViewMode::Lit;
};

struct RhiSmokeEditorViewportState final
{
	bool ViewportEvidenceLogged = false;
	bool SceneColorCaptured = false;
	bool ViewModeOverrideLogged = false;
};

namespace RhiSmokeEditorViewport
{
	void ApplyViewModeOverride(const RhiSmokeEditorViewportConfig& config, RhiSmokeEditorViewportState& state) noexcept;
	void LogEvidence(
	    bool enabled,
	    const ViewportRenderProducts& viewportProducts,
	    const ViewportPresentationProduct& sceneColorPresentation,
	    RhiSmokeEditorViewportState& state) noexcept;
	void CaptureSceneColorIfRequested(
	    bool enabled,
	    const RhiSmokeEditorViewportConfig& config,
	    RuntimeApplication& app,
	    std::uint32_t completedRenderFrames,
	    RhiSmokeEditorViewportState& state,
	    bool& failed) noexcept;
}
