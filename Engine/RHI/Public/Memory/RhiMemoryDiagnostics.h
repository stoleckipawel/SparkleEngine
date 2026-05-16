#pragma once

#include "RhiMemoryTypes.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <filesystem>
#include <string>
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

struct RhiMemoryAllocationInfo
{
	RhiMemoryCategory Category = RhiMemoryCategory::Other;
	RhiMemoryResidencyClass ResidencyClass = RhiMemoryResidencyClass::DeviceLocal;
	std::uint64_t UsedBytes = 0;
	std::uint64_t AllocatedBytes = 0;
	std::wstring DebugName;
};

struct RhiMemoryUsageSnapshot
{
	std::uint64_t TotalUsedBytes = 0;
	std::uint64_t TotalAllocatedBytes = 0;
	std::uint64_t TotalBudgetBytes = 0;
	std::uint64_t ApiUsageBytes = 0;
	std::vector<RhiMemoryCategoryStats> CategoryStats;
	std::vector<RhiMemoryAllocationInfo> Allocations;
};

class SPARKLE_RHI_API RenderMemoryDiagnostics
{
  public:
	virtual ~RenderMemoryDiagnostics() noexcept = default;

	virtual bool SupportsBudgetQueries() const noexcept = 0;
	virtual bool SupportsJsonDump() const noexcept = 0;
	virtual RhiMemoryUsageSnapshot GetLatestMemorySnapshot() const = 0;
	virtual bool WriteAllocatorJsonDump(const std::filesystem::path& outputPath, bool includeDetailedMap = true) const noexcept = 0;
};
