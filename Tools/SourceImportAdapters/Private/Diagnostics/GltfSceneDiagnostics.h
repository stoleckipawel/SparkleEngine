#pragma once

#include "SourceImportDiagnostics.h"

struct cgltf_data;

class GltfSceneDiagnostics final
{
  public:
	static SourceSceneFeatureDiagnostics CaptureFeatures(const cgltf_data* data) noexcept;
};