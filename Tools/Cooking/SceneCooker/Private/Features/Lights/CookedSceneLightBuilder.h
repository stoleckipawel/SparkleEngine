#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportOutput.h"

class CookedSceneLightBuilder final
{
public:
	static void BuildLights(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild);
};
