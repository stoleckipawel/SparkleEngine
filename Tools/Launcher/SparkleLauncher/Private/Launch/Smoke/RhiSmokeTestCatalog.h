#pragma once

#include "SparkleLauncher/LaunchOperations.h"

#include <filesystem>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class RhiSmokeSuite
	{
		SingleViewportCapture,
		RayTracingParity,
		PtlasBenchmark,
		DiagnosticCaptures
	};

	struct RhiSmokeSuiteDefinition final
	{
		RhiSmokeSuite Suite = RhiSmokeSuite::SingleViewportCapture;
		const char* Id = "";
		const char* DisplayName = "";
		const char* ArtifactDirectoryName = "";
		int CaptureFrame = 0;
		int MotionStartFrame = 0;
		int MotionEndFrame = 0;
		int MotionYawDegrees = 0;
		int MotionPitchDegrees = 0;
	};

	struct RhiSmokeScenarioCase final
	{
		const char* Name = "";
		const char* Backend = "";
		bool PreferPartitionedTlas = false;
		const char* PtlasOperationWriterPath = "1";
		const char* RequestedTopLevelMode = "ClassicTlas";
		const char* RequestedWriterPathName = "CpuPack";
		const char* ExpectedSelectedWriterPathName = "CpuPack";
		const char* ExpectedWriterReason = "ptlas-operation-writer-cpu-pack-selected";
		const char* CaptureLabel = "";
	};

	struct RhiSmokeScenarioViewMode final
	{
		const char* Name = "";
		const char* Purpose = "";
	};

	std::vector<RhiSmokeSuite> GetEnabledRhiSmokeSuites(const LaunchOperationPlan& plan);
	std::string GetRhiSmokeSuiteSummary(const LaunchOperationPlan& plan);
	const RhiSmokeSuiteDefinition& GetRhiSmokeSuiteDefinition(RhiSmokeSuite suite);
	const std::vector<RhiSmokeScenarioCase>& GetRhiSmokeCases(RhiSmokeSuite suite);
	const std::vector<RhiSmokeScenarioViewMode>& GetRhiSmokeViewModes(RhiSmokeSuite suite);
	std::filesystem::path GetRhiSmokeValidationDirectory(const LaunchOperationPlan& plan);
	std::filesystem::path GetRhiSmokeArtifactDirectory(const LaunchOperationPlan& plan, RhiSmokeSuite suite);
	std::filesystem::path GetRhiSmokeArtifactPath(
	    const LaunchOperationPlan& plan,
	    RhiSmokeSuite suite,
	    const RhiSmokeScenarioCase& scenarioCase,
	    const RhiSmokeScenarioViewMode& viewMode,
	    std::string_view extension);
}
