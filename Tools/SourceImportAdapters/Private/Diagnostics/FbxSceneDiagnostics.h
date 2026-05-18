#pragma once

#include "SourceImportDiagnostics.h"

struct aiScene;

class FbxSceneDiagnostics final
{
  public:
	static SourceSceneFeatureDiagnostics CaptureFeatures(const aiScene& scene) noexcept;
};