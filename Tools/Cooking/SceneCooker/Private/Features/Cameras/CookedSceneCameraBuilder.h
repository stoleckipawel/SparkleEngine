#pragma once

#include "CookedSceneBuild.h"
#include "SourceImportOutput.h"

class CookedSceneCameraBuilder final
{
public:
	static void BuildCameras(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild);
};
