#pragma once

#include "SourceImportResult.h"

struct cgltf_data;

class GltfMaterialVariantImporter final
{
  public:
	static void ImportMaterialVariants(const cgltf_data* data, SourceImportResult& result);
};
