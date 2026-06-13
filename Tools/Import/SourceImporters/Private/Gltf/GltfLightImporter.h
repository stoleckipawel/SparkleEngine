#pragma once

#include "SourceImportResult.h"

struct cgltf_data;

class GltfLightImporter final
{
  public:
	static void ImportLights(const cgltf_data* data, SourceImportResult& result);
};
