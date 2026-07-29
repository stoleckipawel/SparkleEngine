#pragma once

#include "SourceImportOutput.h"

struct cgltf_data;

class GltfLightImporter final
{
  public:
	static void ImportLights(const cgltf_data* data, SourceImportOutput& output);
};
