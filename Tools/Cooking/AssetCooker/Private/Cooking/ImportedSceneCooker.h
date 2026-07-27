#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

class ImportedSceneCooker final
{
  public:
	static bool Import(
	    const AssetCookerSceneEntry& sceneEntry,
	    AssetCookerCategory category,
	    AssetCookerDiagnostics& diagnostics,
	    SourceImportResult& outImport);
	static bool Build(
	    const AssetCookerSceneEntry& sceneEntry,
	    AssetCookerDiagnostics& diagnostics,
	    CookedSceneBuild& outBuild);

  private:
	static bool BuildCookedScene(
	    const AssetCookerSceneEntry& sceneEntry,
	    const SourceImportResult& importResult,
	    CookedSceneBuild& build,
	    AssetCookerDiagnostics& diagnostics);
};
