#include "Smoke/RhiSmokeTestCatalog.h"

#include "SparkleLauncher/LauncherPaths.h"

namespace SparkleLauncher
{
	RhiSmokeTestCategory GetRhiSmokeTestCategory(const LaunchOperationPlan& plan) noexcept
	{
		if (plan.Kind == LaunchOperationKind::RunRhiRayTracingParitySmoke)
		{
			return RhiSmokeTestCategory::RayTracingParity;
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
		}
		return "Unknown";
	}

	const std::vector<RhiSmokeParityCase>& GetRhiSmokeParityCases()
	{
		static const std::vector<RhiSmokeParityCase> cases = {
		    {"d3d12-classic", "d3d12", false},
		    {"d3d12-ptlas", "d3d12", true},
		    {"vulkan-classic", "vulkan", false},
		    {"vulkan-ptlas", "vulkan", true},
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

	std::filesystem::path GetRhiSmokeParityArtifactDirectory(const LaunchOperationPlan& plan)
	{
		return GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "Latest.txt").parent_path() / "ParityArtifacts";
	}

	std::filesystem::path GetRhiSmokeParityArtifactPath(
	    const LaunchOperationPlan& plan,
	    const RhiSmokeParityCase& parityCase,
	    const RhiSmokeParityViewMode& viewMode,
	    std::string_view extension)
	{
		return GetRhiSmokeParityArtifactDirectory(plan) / parityCase.Name / (std::string(viewMode.Name) + std::string(extension));
	}
}
