#include "Smoke/RhiSmokeTestCatalog.h"

#include "SparkleLauncher/LauncherPaths.h"

namespace SparkleLauncher
{
	std::filesystem::path GetRhiSmokeRayTracingValidationDirectory(const std::filesystem::path& repositoryRoot)
	{
		return GetArtifactDirectory(repositoryRoot) / "validation" / "rhi-raytracing";
	}

	RhiSmokeTestCategory GetRhiSmokeTestCategory(const LaunchOperationPlan& plan) noexcept
	{
		if (plan.Kind == LaunchOperationKind::RunRhiRayTracingParitySmoke)
		{
			return RhiSmokeTestCategory::RayTracingParity;
		}
		if (plan.Kind == LaunchOperationKind::RunRhiRayTracingPtlasBenchmarkSmoke)
		{
			return RhiSmokeTestCategory::RayTracingPtlasBenchmark;
		}
		if (plan.Kind == LaunchOperationKind::RunRhiRayTracingPtlasArticleSmoke)
		{
			return RhiSmokeTestCategory::RayTracingPtlasArticle;
		}
		if (plan.Kind == LaunchOperationKind::RunProject && plan.Request.EnableSmokeTest)
		{
			return RhiSmokeTestCategory::SingleViewportCapture;
		}
		return RhiSmokeTestCategory::None;
	}

	const char* RhiSmokeTestCategoryToString(RhiSmokeTestCategory category) noexcept
	{
		switch (category)
		{
		case RhiSmokeTestCategory::None:
			return "None";
		case RhiSmokeTestCategory::SingleViewportCapture:
			return "SingleViewportCapture";
		case RhiSmokeTestCategory::RayTracingParity:
			return "RayTracingParity";
		case RhiSmokeTestCategory::RayTracingPtlasBenchmark:
			return "RayTracingPtlasBenchmark";
		case RhiSmokeTestCategory::RayTracingPtlasArticle:
			return "RayTracingPtlasArticle";
		}
		return "Unknown";
	}

	const std::vector<RhiSmokeParityCase>& GetRhiSmokeParityCases()
	{
		static const std::vector<RhiSmokeParityCase> cases = {
		    {"d3d12-classic", "d3d12", false, "1", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected"},
		    {"d3d12-ptlas", "d3d12", true, "1", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected"},
		    {"d3d12-ptlas-gpu-logical",
		        "d3d12",
		        true,
		        "2",
		        "GpuLogicalDirtyCpuNativePack",
		        "CpuPack",
		        "ptlas-gpu-logical-dirty-writer-not-implemented"},
		    {"d3d12-ptlas-gpu-native",
		        "d3d12",
		        true,
		        "3",
		        "FullGpuNativePack",
		        "CpuPack",
		        "ptlas-full-gpu-native-pack-backend-pack-shader-not-implemented"},
		    {"vulkan-classic", "vulkan", false, "1", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected"},
		    {"vulkan-ptlas", "vulkan", true, "1", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected"},
		    {"vulkan-ptlas-gpu-logical",
		        "vulkan",
		        true,
		        "2",
		        "GpuLogicalDirtyCpuNativePack",
		        "CpuPack",
		        "ptlas-gpu-logical-dirty-writer-not-implemented"},
		    {"vulkan-ptlas-gpu-native",
		        "vulkan",
		        true,
		        "3",
		        "FullGpuNativePack",
		        "CpuPack",
		        "ptlas-full-gpu-native-pack-backend-pack-shader-not-implemented"},
		};
		return cases;
	}

	const std::vector<RhiSmokeParityViewMode>& GetRhiSmokeParityViewModes()
	{
		static const std::vector<RhiSmokeParityViewMode> viewModes = {
		    {"Lit"},
		    {"RayTracingProviderStatus"},
		};
		return viewModes;
	}

	const std::vector<RhiSmokePtlasBenchmarkCase>& GetRhiSmokePtlasBenchmarkCases()
	{
		static const std::vector<RhiSmokePtlasBenchmarkCase> cases = {
		    {"d3d12-classic", "d3d12", false, "1", "ClassicTlas", "CpuPack"},
		    {"d3d12-ptlas-cpu-pack", "d3d12", true, "1", "PartitionedTlas", "CpuPack"},
		    {"d3d12-ptlas-full-gpu-native", "d3d12", true, "3", "PartitionedTlas", "FullGpuNativePack"},
		    {"vulkan-classic", "vulkan", false, "1", "ClassicTlas", "CpuPack"},
		    {"vulkan-ptlas-cpu-pack", "vulkan", true, "1", "PartitionedTlas", "CpuPack"},
		    {"vulkan-ptlas-gpu-logical", "vulkan", true, "2", "PartitionedTlas", "GpuLogicalDirtyCpuNativePack"},
		    {"vulkan-ptlas-full-gpu-native", "vulkan", true, "3", "PartitionedTlas", "FullGpuNativePack"},
		};
		return cases;
	}

	const std::vector<RhiSmokePtlasBenchmarkViewMode>& GetRhiSmokePtlasBenchmarkViewModes()
	{
		static const std::vector<RhiSmokePtlasBenchmarkViewMode> viewModes = {
		    {"Lit"},
		    {"RayTracingProviderStatus"},
		};
		return viewModes;
	}

	const std::vector<RhiSmokePtlasArticleCase>& GetRhiSmokePtlasArticleCases()
	{
		static const std::vector<RhiSmokePtlasArticleCase> cases = {
		    {"d3d12-classic-reference", "d3d12", false, "1", "ClassicTlas", "CpuPack", "D3D12 classic TLAS reference"},
		    {"d3d12-partitioned-request", "d3d12", true, "1", "PartitionedTlas", "CpuPack", "D3D12 PTLAS request"},
		    {"vulkan-classic-reference", "vulkan", false, "1", "ClassicTlas", "CpuPack", "Vulkan classic TLAS reference"},
		    {"vulkan-partitioned-request", "vulkan", true, "1", "PartitionedTlas", "CpuPack", "Vulkan PTLAS request"},
		};
		return cases;
	}

	const std::vector<RhiSmokePtlasArticleViewMode>& GetRhiSmokePtlasArticleViewModes()
	{
		static const std::vector<RhiSmokePtlasArticleViewMode> viewModes = {
		    {"Lit", "Final lit output used as the primary visual reference for article comparisons."},
		    {"GBufferNormal", "Normal buffer sanity view used to verify orientation, geometry coherence, and backend parity."},
		    {"RayTracingPartitions", "Partition ownership view used to explain logical PTLAS partitioning across the scene."},
		    {"RayTracingPartitionUpdates", "Partition update heatmap showing which logical partitions were touched by motion."},
		    {"RayTracingInstanceMovement", "Instance movement debug view that highlights objects contributing to PTLAS updates."},
		    {"RayTracingTopLevelMode", "Top-level acceleration structure mode view that makes classic TLAS versus PTLAS selection visible."},
		    {"RayTracingNativeOperations", "Native PTLAS operation pressure view used to correlate logical updates with backend-native work."},
		    {"RayTracingGpuDrivenUpdates", "GPU-driven update mode view used to explain writer-path selection and GPU update state."},
		    {"RayTracingProviderStatus", "Provider capability and fallback status view used to explain which backend/provider path is active."},
		};
		return viewModes;
	}

	std::filesystem::path GetRhiSmokeParityArtifactDirectory(const LaunchOperationPlan& plan)
	{
		return GetRhiSmokeRayTracingValidationDirectory(plan.RepositoryRoot) / "parity";
	}

	std::filesystem::path GetRhiSmokeParityArtifactPath(
	    const LaunchOperationPlan& plan,
	    const RhiSmokeParityCase& parityCase,
	    const RhiSmokeParityViewMode& viewMode,
	    std::string_view extension)
	{
		return GetRhiSmokeParityArtifactDirectory(plan) / parityCase.Name / (std::string(viewMode.Name) + std::string(extension));
	}

	std::filesystem::path GetRhiSmokePtlasBenchmarkArtifactDirectory(const LaunchOperationPlan& plan)
	{
		return GetRhiSmokeRayTracingValidationDirectory(plan.RepositoryRoot) / "ptlas-benchmark";
	}

	std::filesystem::path GetRhiSmokePtlasBenchmarkArtifactPath(
	    const LaunchOperationPlan& plan,
	    const RhiSmokePtlasBenchmarkCase& benchmarkCase,
	    const RhiSmokePtlasBenchmarkViewMode& viewMode,
	    std::string_view extension)
	{
		return GetRhiSmokePtlasBenchmarkArtifactDirectory(plan) / benchmarkCase.Name / (std::string(viewMode.Name) + std::string(extension));
	}

	std::filesystem::path GetRhiSmokePtlasArticleArtifactDirectory(const LaunchOperationPlan& plan)
	{
		return GetRhiSmokeRayTracingValidationDirectory(plan.RepositoryRoot) / "ptlas-article";
	}

	std::filesystem::path GetRhiSmokePtlasArticleArtifactPath(
	    const LaunchOperationPlan& plan,
	    const RhiSmokePtlasArticleCase& articleCase,
	    const RhiSmokePtlasArticleViewMode& viewMode,
	    std::string_view extension)
	{
		return GetRhiSmokePtlasArticleArtifactDirectory(plan) / articleCase.Name / (std::string(viewMode.Name) + std::string(extension));
	}
}
