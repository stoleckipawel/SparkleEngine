#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

class CookedSceneMetadataBuilder final
{
  public:
	static void BuildMetadata(const SourceImportResult& importResult, CookedSceneBuild& outBuild);
};
