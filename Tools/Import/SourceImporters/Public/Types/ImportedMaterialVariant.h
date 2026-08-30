#pragma once

#include "ImportedSceneIndices.h"

#include <cstdint>
#include <limits>
#include <string>

using ImportedMaterialVariantIndex = std::uint32_t;

constexpr ImportedMaterialVariantIndex kInvalidImportedMaterialVariantIndex = (std::numeric_limits<ImportedMaterialVariantIndex>::max)();

struct ImportedMaterialVariant
{
	std::string name;
	std::uint32_t sourceVariantIndex = 0;
};

struct ImportedMaterialVariantMapping
{
	std::uint32_t sourceMeshIndex = 0;
	std::uint32_t sourcePrimitiveIndex = 0;
	ImportedMaterialVariantIndex variantIndex = kInvalidImportedMaterialVariantIndex;
	ImportedMaterialIndex materialIndex = kInvalidImportedMaterialIndex;
};
