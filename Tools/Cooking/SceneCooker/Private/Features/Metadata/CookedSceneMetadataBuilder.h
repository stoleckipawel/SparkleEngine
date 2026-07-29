#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportOutput.h"

class CookedSceneMetadataBuilder final
{
  public:
	static void BuildMetadata(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild);
};
