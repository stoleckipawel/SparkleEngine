#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportOutput.h"

class CookedSceneInstanceBuilder final
{
  public:
	static void BuildInstances(const SourceImportOutput& importOutput, CookedSceneBuild& build);
};
