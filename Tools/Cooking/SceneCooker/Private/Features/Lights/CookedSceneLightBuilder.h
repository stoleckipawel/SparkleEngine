#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

class CookedSceneLightBuilder final
{
  public:
	static void BuildLights(const SourceImportResult& importResult, CookedSceneBuild& outBuild);
};
