#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportResult.h"

class CookedSceneCameraBuilder final
{
  public:
	static void BuildCameras(const SourceImportResult& importResult, CookedSceneBuild& outBuild);
};
