#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportOutput.h"

class CookedSceneMaterialVariantBuilder final
{
  public:
	static void BuildMaterialVariants(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild);
};
