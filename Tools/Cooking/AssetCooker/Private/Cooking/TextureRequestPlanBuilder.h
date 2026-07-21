#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include <filesystem>

namespace TextureRequestPlanBuilder
{
	bool Build(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    const std::filesystem::path& outputPath);
}
