#pragma once

#include "SourceImportOutput.h"

struct cgltf_data;

class GltfMaterialVariantImporter final
{
public:
	static void ImportMaterialVariants(const cgltf_data* data, SourceImportOutput& output);
};
