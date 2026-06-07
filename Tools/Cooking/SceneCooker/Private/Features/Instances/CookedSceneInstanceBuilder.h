#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

#include <string>

class CookedSceneInstanceBuilder final
{
  public:
	static bool BuildInstances(
	    const SourceImportResult& importResult,
	    CookedSceneBuild& build,
	    std::string& outErrorMessage);
};
