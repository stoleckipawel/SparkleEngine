#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

#include <string>

class CookedSceneMaterialVariantBuilder final
{
  public:
	static bool BuildMaterialVariants(
	    const SourceImportResult& importResult,
	    CookedSceneBuild& outBuild,
	    std::string& outErrorMessage);
};
