#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

struct ImportedSceneCookProduct final
{
	CookedSceneBuild Scene;
};

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
	    ImportedSceneCookProduct& outProduct);

  private:
	static bool BuildCookedScene(
	    const AssetCookerSceneEntry& sceneEntry,
	    const SourceImportResult& importResult,
	    CookedSceneBuild& build,
	    AssetCookerDiagnostics& diagnostics);
};
