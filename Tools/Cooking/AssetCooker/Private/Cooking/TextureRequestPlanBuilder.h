#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include <filesystem>

class TextureCookRequestSet;

class TextureRequestPlanBuilder final
{
  public:
	TextureRequestPlanBuilder() = delete;

	static bool Build(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    const std::filesystem::path& outputPath);

  private:
	static void CollectSceneRequests(
	    const AssetCookerSceneEntry& sceneEntry,
	    TextureCookRequestSet& requestSet);
};
