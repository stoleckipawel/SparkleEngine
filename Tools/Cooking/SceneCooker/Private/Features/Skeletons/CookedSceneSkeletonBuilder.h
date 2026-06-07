#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

#include <string_view>

class CookedSceneSkeletonBuilder final
{
  public:
	CookedSceneSkeletonBuilder() = delete;
	~CookedSceneSkeletonBuilder() = delete;

	static void BuildSkeletons(const SourceImportResult& importResult, std::string_view sceneAssetId, CookedSceneBuild& outBuild);
};
