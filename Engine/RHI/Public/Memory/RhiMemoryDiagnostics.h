#pragma once

#include "../Core/RhiCapabilities.h"
#include "RhiMemoryTypes.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <vector>

struct RhiMemoryCategoryStats
{
	RhiMemoryCategory Category = RhiMemoryCategory::Other;
	RhiMemoryResidencyClass ResidencyClass = RhiMemoryResidencyClass::DeviceLocal;
	std::uint64_t AllocationCount = 0;
	std::uint64_t ResourceCount = 0;
	std::uint64_t BlockCount = 0;
	std::uint64_t UsedBytes = 0;
	std::uint64_t AllocatedBytes = 0;
	std::uint64_t BudgetBytes = 0;
};

struct RhiMemoryUsageSnapshot
{
	ERhiMemoryAllocatorBackend AllocatorBackend = ERhiMemoryAllocatorBackend::Unknown;
	bool HasBudgetData = false;
	bool HasDelayedDestructionTracking = false;
	std::uint64_t TotalUsedBytes = 0;
	std::uint64_t TotalAllocatedBytes = 0;
	std::uint64_t TotalBudgetBytes = 0;
	std::uint64_t ApiUsageBytes = 0;
	std::uint64_t CommittedUsageBytes = 0;
	std::uint64_t PlacedUsageBytes = 0;
	std::uint64_t TransientUsageBytes = 0;
	std::uint64_t DelayedDestructionBytes = 0;
	std::uint64_t DelayedDestructionAllocationCount = 0;
	std::vector<RhiMemoryCategoryStats> CategoryStats;
};

class SPARKLE_RHI_API RenderMemoryDiagnostics
{
public:
	virtual ~RenderMemoryDiagnostics() noexcept = default;
	RenderMemoryDiagnostics(const RenderMemoryDiagnostics&) = delete;
	RenderMemoryDiagnostics& operator=(const RenderMemoryDiagnostics&) = delete;
	RenderMemoryDiagnostics(RenderMemoryDiagnostics&&) = delete;
	RenderMemoryDiagnostics& operator=(RenderMemoryDiagnostics&&) = delete;

	virtual bool SupportsBudgetQueries() const noexcept = 0;
	virtual bool SupportsDelayedDestructionTracking() const noexcept = 0;
	virtual RhiMemoryUsageSnapshot GetLatestMemorySnapshot() const = 0;

protected:
	RenderMemoryDiagnostics() noexcept = default;
};
