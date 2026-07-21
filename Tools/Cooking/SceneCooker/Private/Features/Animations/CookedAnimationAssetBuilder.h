#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

#include <string_view>

class CookedAnimationAssetBuilder final
{
  public:
	static void Build(const SourceImportResult& importResult, std::string_view sceneAssetId, CookedSceneBuild& outBuild);
};
