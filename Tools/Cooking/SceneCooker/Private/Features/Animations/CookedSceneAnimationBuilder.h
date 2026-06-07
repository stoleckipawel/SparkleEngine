#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

#include <string_view>

class CookedSceneAnimationBuilder final
{
  public:
	static void BuildAnimations(const SourceImportResult& importResult, std::string_view sceneAssetId, CookedSceneBuild& outBuild);
};
