#pragma once

#include "RendererAPI.h"
#include "../../../RHI/Public/Core/RhiBackendApi.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

struct SPARKLE_RENDERER_API RendererGpuTimingMetric final
{
	std::string Label;
	std::uint64_t BeginTicks = 0;
	std::uint64_t EndTicks = 0;
	std::uint64_t DurationTicks = 0;
	double DurationMilliseconds = 0.0;
	std::uint16_t Depth = 0;
};

struct SPARKLE_RENDERER_API RendererFrameTimingDiagnosticsSnapshot final
{
	bool GpuTimingAvailable = false;
	std::vector<RendererGpuTimingMetric> GpuTimings;
};

inline const char* RendererBackendApiToString(ERhiBackendApi backend) noexcept
{
	switch (backend)
	{
		case ERhiBackendApi::D3D12:
			return "D3D12";
		case ERhiBackendApi::Vulkan:
			return "Vulkan";
		case ERhiBackendApi::Unknown:
		default:
			return "Unknown";
	}
}

inline const RendererGpuTimingMetric* FindRendererGpuTiming(
    const RendererFrameTimingDiagnosticsSnapshot& snapshot,
    std::string_view label) noexcept
{
	for (const RendererGpuTimingMetric& timing : snapshot.GpuTimings)
	{
		if (timing.Label == label)
		{
			return &timing;
		}
	}

	return nullptr;
}

inline bool TryGetRendererGpuTimingMilliseconds(
    const RendererFrameTimingDiagnosticsSnapshot& snapshot,
    std::string_view label,
    double& outMilliseconds) noexcept
{
	const RendererGpuTimingMetric* timing = FindRendererGpuTiming(snapshot, label);
	if (timing == nullptr)
	{
		return false;
	}

	outMilliseconds = timing->DurationMilliseconds;
	return true;
}
