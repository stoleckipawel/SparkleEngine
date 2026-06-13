#pragma once

#include "SourceImportResult.h"

struct cgltf_data;

class GltfAnimationImporter final
{
  public:
	static void ImportAnimations(const cgltf_data* data, SourceImportResult& result);
};
