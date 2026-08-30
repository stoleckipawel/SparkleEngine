#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include "CookedSceneBuild.h"
#include "SourceImportOutput.h"

class ImportedSceneCooker final
{
public:
	static SourceImportOutput Import(const AssetCookerSceneEntry& sceneEntry);
	static CookedSceneBuild Build(const AssetCookerSceneEntry& sceneEntry, AssetCookerDiagnostics& diagnostics);

private:
	static CookedSceneBuild BuildCookedScene(
	    const AssetCookerSceneEntry& sceneEntry,
	    const SourceImportOutput& importOutput,
	    AssetCookerDiagnostics& diagnostics);
};
