#pragma once

#include "Types/ImportedMaterial.h"

struct cgltf_material;

class GltfMaterialPropertyMapper final
{
public:
	static void Apply(const cgltf_material& material, ImportedMaterial& importedMaterial);
};
