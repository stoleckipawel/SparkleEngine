#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class AssetCookerStageExecutor final
{
  public:
	AssetCookerStageExecutor() = delete;

	static const char* GetStepName(AssetCookerPlanStep step) noexcept;
	static bool ValidateCapabilities(const AssetCookerProjectCookPlan& plan, AssetCookerDiagnostics& diagnostics);
	static bool Execute(
	    AssetCookerPlanStep step,
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outOutputs);

  private:
	static bool FileExists(const std::filesystem::path& path);
	static bool PlanUsesStep(const AssetCookerProjectCookPlan& plan, AssetCookerPlanStep step) noexcept;
	static std::filesystem::path ResolveToolPath(
	    const AssetCookerProjectCookPlan& plan,
	    std::string_view executableName);
	static std::filesystem::path MakeTemporaryPath(
	    const AssetCookerProjectCookPlan& plan,
	    std::string_view stem,
	    std::string_view extension);
	static void AppendOutput(
	    std::vector<AssetCookerOutputRecord>& outputs,
	    AssetCookerCategory category,
	    std::string assetId,
	    std::filesystem::path path);
	static bool RunShaders(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs);
	static bool RunTextures(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs);
	static bool RunSceneAssets(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs);
};
