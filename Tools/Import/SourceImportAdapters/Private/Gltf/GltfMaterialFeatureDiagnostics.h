#pragma once

#include "SourceImportResult.h"

struct cgltf_material;

class GltfMaterialFeatureDiagnostics final
{
  public:
	static void ReportUnsupportedFeatures(const cgltf_material& material, ImportedMaterialIndex materialIndex, SourceImportResult& result);
};
