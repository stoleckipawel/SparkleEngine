#pragma once

#include "../RendererAPI.h"
#include "../../../RHI/Public/Core/RhiBackendApi.h"

#include <cstdint>
#include <string>

struct SPARKLE_RENDERER_API RendererSmokeDiagnosticsSnapshot final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
	std::uint32_t FrameGraphUnresolvedBarrierWarnings = 0;
	std::string UpscalerProvider;
	std::string UpscalerStatus;
	std::string UpscalerReason;
	bool RayTracingSupported = false;
	bool InlineRayQuerySupported = false;
};
