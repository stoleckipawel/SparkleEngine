#pragma once

#include "Renderer/Public/Diagnostics/RendererMemoryDiagnostics.h"

#include <cstddef>
#include <cstdint>

class RenderDiagnostics;

class RendererMemoryMonitor final
{
  public:
	explicit RendererMemoryMonitor(RenderDiagnostics& diagnostics, std::uint32_t pollIntervalFrames = 30) noexcept;

	void Tick(std::uint64_t frameIndex);

	const RendererMemoryDiagnosticsSnapshot& GetLatestSnapshot() const noexcept { return m_latestSnapshot; }

  private:
	static RendererMemoryPressureLevel ClassifyPressure(float usageRatio) noexcept;
	static RendererMemoryPressureLevel MaxPressure(RendererMemoryPressureLevel lhs, RendererMemoryPressureLevel rhs) noexcept;
	static bool IsAtLeast(RendererMemoryPressureLevel value, RendererMemoryPressureLevel threshold) noexcept;
	static TextureStreamingMemoryPolicySnapshot BuildTextureStreamingPolicy(
	    RendererMemoryPressureLevel overallPressure,
	    const std::vector<RendererMemoryCategoryPressure>& categoryPressure);
	static SceneMemoryReport BuildSceneMemoryReport(const RhiMemoryUsageSnapshot& memoryUsage);
	static bool HasReachedPollInterval(std::uint64_t frameIndex, std::uint64_t lastPollFrame, std::uint32_t pollIntervalFrames) noexcept;
	static constexpr std::size_t kLargestNamedAllocationCount = 16;

	RenderDiagnostics* m_diagnostics = nullptr;
	RendererMemoryDiagnosticsSnapshot m_latestSnapshot;
	bool m_hasPolled = false;
};
