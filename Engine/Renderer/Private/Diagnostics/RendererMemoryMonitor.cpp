#include "PCH.h"

#include "Diagnostics/RendererMemoryMonitor.h"

#include "RHI/Public/Diagnostics/RhiDiagnostics.h"

#include <algorithm>
#include <utility>

const char* RendererMemoryPressureLevelToString(RendererMemoryPressureLevel pressure) noexcept
{
	switch (pressure)
	{
		case RendererMemoryPressureLevel::Normal:
			return "Normal";
		case RendererMemoryPressureLevel::Watch:
			return "Watch";
		case RendererMemoryPressureLevel::Pressure:
			return "Pressure";
		case RendererMemoryPressureLevel::Critical:
			return "Critical";
		default:
			return "Unknown";
	}
}

RendererMemoryMonitor::RendererMemoryMonitor(RenderDiagnostics& diagnostics, std::uint32_t pollIntervalFrames) noexcept :
    m_diagnostics(&diagnostics)
{
	m_latestSnapshot.PollIntervalFrames = pollIntervalFrames;
}

void RendererMemoryMonitor::Tick(std::uint64_t frameIndex)
{
	if (m_diagnostics == nullptr ||
	    (m_hasPolled && !HasReachedPollInterval(frameIndex, m_latestSnapshot.LastPollFrame, m_latestSnapshot.PollIntervalFrames)))
	{
		return;
	}

	RenderMemoryDiagnostics* memoryDiagnostics = m_diagnostics->GetMemoryDiagnostics();
	if (memoryDiagnostics == nullptr || !memoryDiagnostics->SupportsBudgetQueries())
	{
		m_latestSnapshot.Available = false;
		m_latestSnapshot.LastPollFrame = frameIndex;
		m_hasPolled = true;
		return;
	}

	RendererMemoryDiagnosticsSnapshot snapshot;
	snapshot.Available = true;
	snapshot.LastPollFrame = frameIndex;
	snapshot.PollIntervalFrames = m_latestSnapshot.PollIntervalFrames;
	snapshot.MemoryUsage = memoryDiagnostics->GetLatestMemorySnapshot();
	snapshot.CategoryPressure.reserve(snapshot.MemoryUsage.CategoryStats.size());

	RendererMemoryPressureLevel overallPressure = RendererMemoryPressureLevel::Normal;
	for (const RhiMemoryCategoryStats& categoryStats : snapshot.MemoryUsage.CategoryStats)
	{
		const float budgetUsageRatio = categoryStats.BudgetBytes != 0
		                                 ? static_cast<float>(categoryStats.UsedBytes) / static_cast<float>(categoryStats.BudgetBytes)
		                                 : 0.0f;
		const RendererMemoryPressureLevel pressure = ClassifyPressure(budgetUsageRatio);
		overallPressure = MaxPressure(overallPressure, pressure);
		snapshot.CategoryPressure.push_back(RendererMemoryCategoryPressure{
		    .Category = categoryStats.Category,
		    .ResidencyClass = categoryStats.ResidencyClass,
		    .UsedBytes = categoryStats.UsedBytes,
		    .BudgetBytes = categoryStats.BudgetBytes,
		    .BudgetUsageRatio = budgetUsageRatio,
		    .Pressure = pressure});
	}

	snapshot.OverallPressure = overallPressure;
	snapshot.TextureStreamingPolicy = BuildTextureStreamingPolicy(overallPressure, snapshot.CategoryPressure);
	snapshot.SceneReport = BuildSceneMemoryReport(snapshot.MemoryUsage);
	m_latestSnapshot = std::move(snapshot);
	m_hasPolled = true;
}

RendererMemoryPressureLevel RendererMemoryMonitor::ClassifyPressure(float usageRatio) noexcept
{
	const RendererMemoryPressureThresholds thresholds;
	if (usageRatio >= thresholds.CriticalRatio)
	{
		return RendererMemoryPressureLevel::Critical;
	}
	if (usageRatio >= thresholds.PressureRatio)
	{
		return RendererMemoryPressureLevel::Pressure;
	}
	if (usageRatio >= thresholds.WatchRatio)
	{
		return RendererMemoryPressureLevel::Watch;
	}
	return RendererMemoryPressureLevel::Normal;
}

RendererMemoryPressureLevel RendererMemoryMonitor::MaxPressure(
    RendererMemoryPressureLevel lhs,
    RendererMemoryPressureLevel rhs) noexcept
{
	return static_cast<std::uint8_t>(lhs) >= static_cast<std::uint8_t>(rhs) ? lhs : rhs;
}

bool RendererMemoryMonitor::IsAtLeast(RendererMemoryPressureLevel value, RendererMemoryPressureLevel threshold) noexcept
{
	return static_cast<std::uint8_t>(value) >= static_cast<std::uint8_t>(threshold);
}

TextureStreamingMemoryPolicySnapshot RendererMemoryMonitor::BuildTextureStreamingPolicy(
    RendererMemoryPressureLevel overallPressure,
    const std::vector<RendererMemoryCategoryPressure>& categoryPressure)
{
	RendererMemoryPressureLevel texturePressure = RendererMemoryPressureLevel::Normal;
	for (const RendererMemoryCategoryPressure& pressure : categoryPressure)
	{
		if (pressure.Category == RhiMemoryCategory::Texture)
		{
			texturePressure = MaxPressure(texturePressure, pressure.Pressure);
		}
	}

	return TextureStreamingMemoryPolicySnapshot{
	    .OverallPressure = overallPressure,
	    .TexturePressure = texturePressure,
	    .ShouldConserveTextureMemory = IsAtLeast(texturePressure, RendererMemoryPressureLevel::Watch) ||
	                                  IsAtLeast(overallPressure, RendererMemoryPressureLevel::Pressure),
	    .ShouldPreferMipDemotion = IsAtLeast(texturePressure, RendererMemoryPressureLevel::Pressure) ||
	                               IsAtLeast(overallPressure, RendererMemoryPressureLevel::Critical),
	    .ShouldBlockMipPromotion = IsAtLeast(texturePressure, RendererMemoryPressureLevel::Critical)};
}

SceneMemoryReport RendererMemoryMonitor::BuildSceneMemoryReport(const RhiMemoryUsageSnapshot& memoryUsage)
{
	SceneMemoryReport report;
	for (const RhiMemoryCategoryStats& categoryStats : memoryUsage.CategoryStats)
	{
		report.TotalTrackedBytes += categoryStats.UsedBytes;
		switch (categoryStats.Category)
		{
			case RhiMemoryCategory::Texture:
				report.TextureBytes += categoryStats.UsedBytes;
				break;
			case RhiMemoryCategory::Mesh:
				report.MeshBytes += categoryStats.UsedBytes;
				break;
			case RhiMemoryCategory::RayTracing:
				report.RayTracingBytes += categoryStats.UsedBytes;
				break;
			case RhiMemoryCategory::Upload:
				report.UploadBytes += categoryStats.UsedBytes;
				break;
			case RhiMemoryCategory::ConstantBuffer:
				report.ConstantBufferBytes += categoryStats.UsedBytes;
				break;
			default:
				break;
		}
	}

	for (const RhiMemoryAllocationInfo& allocation : memoryUsage.Allocations)
	{
		if (!allocation.DebugName.empty())
		{
			++report.NamedAllocationCount;
			report.LargestNamedAllocations.push_back(allocation);
		}
	}

	std::sort(
	    report.LargestNamedAllocations.begin(),
	    report.LargestNamedAllocations.end(),
	    [](const RhiMemoryAllocationInfo& lhs, const RhiMemoryAllocationInfo& rhs) noexcept
	    {
		    if (lhs.UsedBytes != rhs.UsedBytes)
		    {
			    return lhs.UsedBytes > rhs.UsedBytes;
		    }
		    return lhs.DebugName < rhs.DebugName;
	    });
	if (report.LargestNamedAllocations.size() > kLargestNamedAllocationCount)
	{
		report.LargestNamedAllocations.resize(kLargestNamedAllocationCount);
	}

	return report;
}

bool RendererMemoryMonitor::HasReachedPollInterval(
    std::uint64_t frameIndex,
    std::uint64_t lastPollFrame,
    std::uint32_t pollIntervalFrames) noexcept
{
	if (pollIntervalFrames == 0)
	{
		return true;
	}
	return frameIndex >= lastPollFrame && frameIndex - lastPollFrame >= pollIntervalFrames;
}
