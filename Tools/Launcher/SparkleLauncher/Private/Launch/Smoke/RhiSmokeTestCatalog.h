#pragma once

#include "SparkleLauncher/LaunchOperations.h"

#include <filesystem>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class RhiSmokeTestCategory
	{
		None,
		SingleViewportCapture,
		RayTracingParity
	};

	struct RhiSmokeParityCase final
	{
		const char* Name = "";
		const char* Backend = "";
		bool PreferPartitionedTlas = false;
		const char* PtlasOperationWriterPath = "1";
		const char* ExpectedRequestedWriterPathName = "CpuPack";
	};

	struct RhiSmokeParityViewMode final
	{
		const char* Name = "";
	};

	RhiSmokeTestCategory GetRhiSmokeTestCategory(const LaunchOperationPlan& plan) noexcept;
	const char* RhiSmokeTestCategoryToString(RhiSmokeTestCategory category) noexcept;
	const std::vector<RhiSmokeParityCase>& GetRhiSmokeParityCases();
	const std::vector<RhiSmokeParityViewMode>& GetRhiSmokeParityViewModes();
	std::filesystem::path GetRhiSmokeParityArtifactDirectory(const LaunchOperationPlan& plan);
	std::filesystem::path GetRhiSmokeParityArtifactPath(
	    const LaunchOperationPlan& plan,
	    const RhiSmokeParityCase& parityCase,
	    const RhiSmokeParityViewMode& viewMode,
	    std::string_view extension);
}
