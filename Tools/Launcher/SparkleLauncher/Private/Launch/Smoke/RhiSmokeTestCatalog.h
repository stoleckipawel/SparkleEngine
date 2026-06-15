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
		RayTracingParity,
		RayTracingPtlasBenchmark,
		RayTracingPtlasArticle
	};

	struct RhiSmokeParityCase final
	{
		const char* Name = "";
		const char* Backend = "";
		bool PreferPartitionedTlas = false;
		const char* PtlasOperationWriterPath = "1";
		const char* ExpectedRequestedWriterPathName = "CpuPack";
		const char* ExpectedSelectedWriterPathName = "CpuPack";
		const char* ExpectedWriterReason = "ptlas-operation-writer-cpu-pack-selected";
	};

	struct RhiSmokeParityViewMode final
	{
		const char* Name = "";
	};

	struct RhiSmokePtlasBenchmarkCase final
	{
		const char* Name = "";
		const char* Backend = "";
		bool PreferPartitionedTlas = false;
		const char* PtlasOperationWriterPath = "1";
		const char* RequestedTopLevelMode = "ClassicTlas";
		const char* RequestedWriterPathName = "CpuPack";
	};

	struct RhiSmokePtlasBenchmarkViewMode final
	{
		const char* Name = "";
	};

	struct RhiSmokePtlasArticleCase final
	{
		const char* Name = "";
		const char* Backend = "";
		bool PreferPartitionedTlas = false;
		const char* PtlasOperationWriterPath = "1";
		const char* RequestedTopLevelMode = "ClassicTlas";
		const char* RequestedWriterPathName = "CpuPack";
		const char* StoryLabel = "";
	};

	struct RhiSmokePtlasArticleViewMode final
	{
		const char* Name = "";
		const char* Purpose = "";
	};

	RhiSmokeTestCategory GetRhiSmokeTestCategory(const LaunchOperationPlan& plan) noexcept;
	const char* RhiSmokeTestCategoryToString(RhiSmokeTestCategory category) noexcept;
	const std::vector<RhiSmokeParityCase>& GetRhiSmokeParityCases();
	const std::vector<RhiSmokeParityViewMode>& GetRhiSmokeParityViewModes();
	const std::vector<RhiSmokePtlasBenchmarkCase>& GetRhiSmokePtlasBenchmarkCases();
	const std::vector<RhiSmokePtlasBenchmarkViewMode>& GetRhiSmokePtlasBenchmarkViewModes();
	const std::vector<RhiSmokePtlasArticleCase>& GetRhiSmokePtlasArticleCases();
	const std::vector<RhiSmokePtlasArticleViewMode>& GetRhiSmokePtlasArticleViewModes();
	std::filesystem::path GetRhiSmokeParityArtifactDirectory(const LaunchOperationPlan& plan);
	std::filesystem::path GetRhiSmokeParityArtifactPath(
	    const LaunchOperationPlan& plan,
	    const RhiSmokeParityCase& parityCase,
	    const RhiSmokeParityViewMode& viewMode,
	    std::string_view extension);
	std::filesystem::path GetRhiSmokePtlasBenchmarkArtifactDirectory(const LaunchOperationPlan& plan);
	std::filesystem::path GetRhiSmokePtlasBenchmarkArtifactPath(
	    const LaunchOperationPlan& plan,
	    const RhiSmokePtlasBenchmarkCase& benchmarkCase,
	    const RhiSmokePtlasBenchmarkViewMode& viewMode,
	    std::string_view extension);
	std::filesystem::path GetRhiSmokePtlasArticleArtifactDirectory(const LaunchOperationPlan& plan);
	std::filesystem::path GetRhiSmokePtlasArticleArtifactPath(
	    const LaunchOperationPlan& plan,
	    const RhiSmokePtlasArticleCase& articleCase,
	    const RhiSmokePtlasArticleViewMode& viewMode,
	    std::string_view extension);
}
