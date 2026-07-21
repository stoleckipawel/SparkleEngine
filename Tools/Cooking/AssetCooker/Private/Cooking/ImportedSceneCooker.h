#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include <functional>

class SourceImportResult;

namespace ImportedSceneCooker
{
	using SceneVisitor = std::function<bool(const SourceImportResult&)>;

	bool ImportAndVisit(
	    const AssetCookerSceneEntry& sceneEntry,
	    AssetCookerCategory category,
	    AssetCookerDiagnostics& diagnostics,
	    const SceneVisitor& visitor);

	bool Cook(
	    const AssetCookerSceneEntry& sceneEntry,
	    const SourceImportResult& importResult,
	    AssetCookerDiagnostics& diagnostics);
}
