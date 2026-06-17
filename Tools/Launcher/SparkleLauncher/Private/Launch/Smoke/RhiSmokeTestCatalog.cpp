#include "Smoke/RhiSmokeTestCatalog.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <sstream>

namespace SparkleLauncher
{
	namespace
	{
		const RhiSmokeSuiteDefinition& SingleViewportSuiteDefinition()
		{
			static const RhiSmokeSuiteDefinition definition = {
			    RhiSmokeSuite::SingleViewportCapture,
			    "single-viewport",
			    "Single viewport smoke",
			    "single-viewport",
			    50,
			    0,
			    0,
			    0,
			    0};
			return definition;
		}

		const RhiSmokeSuiteDefinition& ParitySuiteDefinition()
		{
			static const RhiSmokeSuiteDefinition definition = {
			    RhiSmokeSuite::RayTracingParity,
			    "parity",
			    "Ray tracing parity",
			    "parity",
			    50,
			    10,
			    40,
			    12,
			    0};
			return definition;
		}

		const RhiSmokeSuiteDefinition& BenchmarkSuiteDefinition()
		{
			static const RhiSmokeSuiteDefinition definition = {
			    RhiSmokeSuite::PtlasBenchmark,
			    "ptlas-benchmark",
			    "PTLAS benchmark",
			    "ptlas-benchmark",
			    80,
			    10,
			    70,
			    24,
			    0};
			return definition;
		}

		const RhiSmokeSuiteDefinition& DiagnosticSuiteDefinition()
		{
			static const RhiSmokeSuiteDefinition definition = {
			    RhiSmokeSuite::DiagnosticCaptures,
			    "diagnostic-captures",
			    "PTLAS diagnostic captures",
			    "diagnostic-captures",
			    80,
			    10,
			    70,
			    24,
			    0};
			return definition;
		}
	}

	std::vector<RhiSmokeSuite> GetEnabledRhiSmokeSuites(const LaunchOperationPlan& plan)
	{
		std::vector<RhiSmokeSuite> suites;
		if (!plan.Request.EnableSmokeTest)
		{
			return suites;
		}

		if (plan.Request.SmokeRunRayTracingParity)
		{
			suites.push_back(RhiSmokeSuite::RayTracingParity);
		}
		if (plan.Request.SmokeRunPtlasBenchmark)
		{
			suites.push_back(RhiSmokeSuite::PtlasBenchmark);
		}
		if (plan.Request.SmokeRunDiagnosticCaptures)
		{
			suites.push_back(RhiSmokeSuite::DiagnosticCaptures);
		}
		if (suites.empty())
		{
			suites.push_back(RhiSmokeSuite::SingleViewportCapture);
		}

		return suites;
	}

	std::string GetRhiSmokeSuiteSummary(const LaunchOperationPlan& plan)
	{
		const std::vector<RhiSmokeSuite> suites = GetEnabledRhiSmokeSuites(plan);
		std::ostringstream stream;
		for (std::size_t index = 0; index < suites.size(); ++index)
		{
			if (index > 0)
			{
				stream << ", ";
			}
			stream << GetRhiSmokeSuiteDefinition(suites[index]).Id;
		}
		return stream.str();
	}

	const RhiSmokeSuiteDefinition& GetRhiSmokeSuiteDefinition(RhiSmokeSuite suite)
	{
		switch (suite)
		{
		case RhiSmokeSuite::SingleViewportCapture:
			return SingleViewportSuiteDefinition();
		case RhiSmokeSuite::RayTracingParity:
			return ParitySuiteDefinition();
		case RhiSmokeSuite::PtlasBenchmark:
			return BenchmarkSuiteDefinition();
		case RhiSmokeSuite::DiagnosticCaptures:
			return DiagnosticSuiteDefinition();
		}

		return SingleViewportSuiteDefinition();
	}

	const std::vector<RhiSmokeScenarioCase>& GetRhiSmokeCases(RhiSmokeSuite suite)
	{
		static const std::vector<RhiSmokeScenarioCase> singleViewportCases = {
		    {"selected-launch", "", false, "1", "ClassicTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		};
		static const std::vector<RhiSmokeScenarioCase> parityCases = {
		    {"d3d12-classic", "d3d12", false, "1", "ClassicTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		    {"d3d12-ptlas", "d3d12", true, "1", "PartitionedTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		    {"d3d12-ptlas-gpu-logical", "d3d12", true, "2", "PartitionedTlas", "GpuLogicalDirtyCpuNativePack", "CpuPack", "ptlas-gpu-logical-dirty-writer-not-implemented", ""},
		    {"d3d12-ptlas-gpu-native", "d3d12", true, "3", "PartitionedTlas", "FullGpuNativePack", "CpuPack", "ptlas-full-gpu-native-pack-backend-pack-shader-not-implemented", ""},
		    {"vulkan-classic", "vulkan", false, "1", "ClassicTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		    {"vulkan-ptlas", "vulkan", true, "1", "PartitionedTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		    {"vulkan-ptlas-gpu-logical", "vulkan", true, "2", "PartitionedTlas", "GpuLogicalDirtyCpuNativePack", "CpuPack", "ptlas-gpu-logical-dirty-writer-not-implemented", ""},
		    {"vulkan-ptlas-gpu-native", "vulkan", true, "3", "PartitionedTlas", "FullGpuNativePack", "CpuPack", "ptlas-full-gpu-native-pack-backend-pack-shader-not-implemented", ""},
		};
		static const std::vector<RhiSmokeScenarioCase> benchmarkCases = {
		    {"d3d12-classic", "d3d12", false, "1", "ClassicTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		    {"d3d12-ptlas-cpu-pack", "d3d12", true, "1", "PartitionedTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		    {"d3d12-ptlas-full-gpu-native", "d3d12", true, "3", "PartitionedTlas", "FullGpuNativePack", "CpuPack", "ptlas-full-gpu-native-pack-backend-pack-shader-not-implemented", ""},
		    {"vulkan-classic", "vulkan", false, "1", "ClassicTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		    {"vulkan-ptlas-cpu-pack", "vulkan", true, "1", "PartitionedTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", ""},
		    {"vulkan-ptlas-gpu-logical", "vulkan", true, "2", "PartitionedTlas", "GpuLogicalDirtyCpuNativePack", "CpuPack", "ptlas-gpu-logical-dirty-writer-not-implemented", ""},
		    {"vulkan-ptlas-full-gpu-native", "vulkan", true, "3", "PartitionedTlas", "FullGpuNativePack", "CpuPack", "ptlas-full-gpu-native-pack-backend-pack-shader-not-implemented", ""},
		};
		static const std::vector<RhiSmokeScenarioCase> diagnosticCases = {
		    {"d3d12-classic-reference", "d3d12", false, "1", "ClassicTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", "D3D12 classic TLAS reference"},
		    {"d3d12-partitioned-request", "d3d12", true, "1", "PartitionedTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", "D3D12 partitioned TLAS request"},
		    {"vulkan-classic-reference", "vulkan", false, "1", "ClassicTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", "Vulkan classic TLAS reference"},
		    {"vulkan-partitioned-request", "vulkan", true, "1", "PartitionedTlas", "CpuPack", "CpuPack", "ptlas-operation-writer-cpu-pack-selected", "Vulkan partitioned TLAS request"},
		};

		switch (suite)
		{
		case RhiSmokeSuite::SingleViewportCapture:
			return singleViewportCases;
		case RhiSmokeSuite::RayTracingParity:
			return parityCases;
		case RhiSmokeSuite::PtlasBenchmark:
			return benchmarkCases;
		case RhiSmokeSuite::DiagnosticCaptures:
			return diagnosticCases;
		}

		return singleViewportCases;
	}

	const std::vector<RhiSmokeScenarioViewMode>& GetRhiSmokeViewModes(RhiSmokeSuite suite)
	{
		static const std::vector<RhiSmokeScenarioViewMode> singleViewportViewModes = {
		    {"Selected", "Launcher-selected smoke validation view."},
		};
		static const std::vector<RhiSmokeScenarioViewMode> parityViewModes = {
		    {"Lit", "Primary shaded output used for backend and PTLAS parity comparisons."},
		    {"RayTracingProviderStatus", "Provider capability and fallback status view used to validate active PTLAS provider selection."},
		};
		static const std::vector<RhiSmokeScenarioViewMode> benchmarkViewModes = {
		    {"Lit", "Primary shaded output used for timing comparisons and benchmark summaries."},
		    {"RayTracingProviderStatus", "Provider capability view used to correlate timing data with selected PTLAS provider state."},
		};
		static const std::vector<RhiSmokeScenarioViewMode> diagnosticViewModes = {
		    {"Lit", "Final lit output used as the primary visual reference."},
		    {"GBufferNormal", "Normal buffer sanity view used to verify orientation and geometry coherence."},
		    {"RayTracingPartitions", "Partition ownership view used to inspect logical PTLAS partitioning."},
		    {"RayTracingPartitionUpdates", "Partition update heatmap showing which logical partitions were touched by motion."},
		    {"RayTracingInstanceMovement", "Instance movement debug view that highlights objects contributing to PTLAS updates."},
		    {"RayTracingTopLevelMode", "Top-level acceleration structure mode view exposing classic TLAS versus PTLAS selection."},
		    {"RayTracingNativeOperations", "Native PTLAS operation pressure view for backend-native work inspection."},
		    {"RayTracingGpuDrivenUpdates", "GPU-driven update mode view used to inspect writer-path selection and GPU update state."},
		    {"RayTracingProviderStatus", "Provider capability and fallback status view for active backend/provider diagnostics."},
		};

		switch (suite)
		{
		case RhiSmokeSuite::SingleViewportCapture:
			return singleViewportViewModes;
		case RhiSmokeSuite::RayTracingParity:
			return parityViewModes;
		case RhiSmokeSuite::PtlasBenchmark:
			return benchmarkViewModes;
		case RhiSmokeSuite::DiagnosticCaptures:
			return diagnosticViewModes;
		}

		return singleViewportViewModes;
	}

	std::filesystem::path GetRhiSmokeValidationDirectory(const LaunchOperationPlan& plan)
	{
		return GetArtifactDirectory(plan.RepositoryRoot) / "validation" / "rhi-raytracing";
	}

	std::filesystem::path GetRhiSmokeArtifactDirectory(const LaunchOperationPlan& plan, RhiSmokeSuite suite)
	{
		return GetRhiSmokeValidationDirectory(plan) / GetRhiSmokeSuiteDefinition(suite).ArtifactDirectoryName;
	}

	std::filesystem::path GetRhiSmokeArtifactPath(
	    const LaunchOperationPlan& plan,
	    RhiSmokeSuite suite,
	    const RhiSmokeScenarioCase& scenarioCase,
	    const RhiSmokeScenarioViewMode& viewMode,
	    std::string_view extension)
	{
		return GetRhiSmokeArtifactDirectory(plan, suite) / scenarioCase.Name / (std::string(viewMode.Name) + std::string(extension));
	}
}
