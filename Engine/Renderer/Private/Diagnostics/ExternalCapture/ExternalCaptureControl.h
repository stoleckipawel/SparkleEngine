#pragma once

#include "RHI/Public/Diagnostics/RhiExternalCapture.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

// The request carries the displayed viewport generation, not an offscreen native
// device/window. Native lowering binds the existing containing host presentation.
struct RenderExternalCaptureCommand final
{
	std::uint64_t RequestId = 0;
};

void ArmExternalCapture(RhiExternalCapture& capture, const RenderExternalCaptureCommand& command, const ViewportRenderProducts& products) noexcept;

void ValidateExternalCaptureContext(RhiExternalCapture& capture, std::uint64_t currentGeneration, bool sceneReset) noexcept;
