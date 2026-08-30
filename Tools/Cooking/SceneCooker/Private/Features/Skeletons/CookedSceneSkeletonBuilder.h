#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportOutput.h"

#include <string_view>

class CookedSceneSkeletonBuilder final
{
public:
	CookedSceneSkeletonBuilder() = delete;
	~CookedSceneSkeletonBuilder() = delete;

	static void BuildSkeletons(const SourceImportOutput& importOutput, std::string_view sceneAssetId, CookedSceneBuild& outBuild);
};
