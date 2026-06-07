#include "PCH.h"

#include "Gltf/GltfMaterialVariantImporter.h"

#include <cgltf.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <utility>

namespace
{
	ImportedMaterialIndex ResolveImportedMaterialIndex(const cgltf_data* data, const cgltf_material* material) noexcept
	{
		if (data == nullptr || material == nullptr || data->materials == nullptr || data->materials_count == 0)
		{
			return kInvalidImportedMaterialIndex;
		}

		const std::ptrdiff_t materialIndex = material - data->materials;
		if (materialIndex < 0 || static_cast<cgltf_size>(materialIndex) >= data->materials_count)
		{
			return kInvalidImportedMaterialIndex;
		}

		return static_cast<ImportedMaterialIndex>(materialIndex);
	}

	std::string BuildVariantName(const cgltf_material_variant& variant, cgltf_size variantIndex)
	{
		if (variant.name != nullptr && variant.name[0] != '\0')
		{
			return variant.name;
		}

		return std::format("Variant{}", static_cast<std::uint32_t>(variantIndex));
	}

	void ImportVariantSet(const cgltf_data* data, SourceImportResult& result)
	{
		result.scene.materialVariants.reserve(data->variants_count);
		for (cgltf_size variantIndex = 0; variantIndex < data->variants_count; ++variantIndex)
		{
			ImportedMaterialVariant importedVariant;
			importedVariant.name = BuildVariantName(data->variants[variantIndex], variantIndex);
			importedVariant.sourceVariantIndex = static_cast<std::uint32_t>(variantIndex);
			result.scene.materialVariants.push_back(std::move(importedVariant));
		}
	}

	void ImportPrimitiveMappings(
	    const cgltf_data* data,
	    const cgltf_primitive& primitive,
	    std::uint32_t sourceMeshIndex,
	    std::uint32_t sourcePrimitiveIndex,
	    SourceImportResult& result)
	{
		for (cgltf_size mappingIndex = 0; mappingIndex < primitive.mappings_count; ++mappingIndex)
		{
			const cgltf_material_mapping& mapping = primitive.mappings[mappingIndex];
			if (mapping.material == nullptr)
			{
				continue;
			}

			const ImportedMaterialIndex materialIndex = ResolveImportedMaterialIndex(data, mapping.material);
			if (materialIndex == kInvalidImportedMaterialIndex || materialIndex >= result.scene.materials.size())
			{
				continue;
			}

			const cgltf_size sourceVariantIndex = mapping.variant;
			if (sourceVariantIndex >= result.scene.materialVariants.size())
			{
				continue;
			}

			ImportedMaterialVariantMapping importedMapping;
			importedMapping.sourceMeshIndex = sourceMeshIndex;
			importedMapping.sourcePrimitiveIndex = sourcePrimitiveIndex;
			importedMapping.variantIndex = static_cast<ImportedMaterialVariantIndex>(sourceVariantIndex);
			importedMapping.materialIndex = materialIndex;
			result.scene.materialVariantMappings.push_back(importedMapping);
		}
	}
}  // namespace

void GltfMaterialVariantImporter::ImportMaterialVariants(const cgltf_data* data, SourceImportResult& result)
{
	if (data == nullptr || data->variants_count == 0)
	{
		return;
	}

	ImportVariantSet(data, result);
	for (cgltf_size meshIndex = 0; meshIndex < data->meshes_count; ++meshIndex)
	{
		const cgltf_mesh& mesh = data->meshes[meshIndex];
		for (cgltf_size primitiveIndex = 0; primitiveIndex < mesh.primitives_count; ++primitiveIndex)
		{
			ImportPrimitiveMappings(
			    data,
			    mesh.primitives[primitiveIndex],
			    static_cast<std::uint32_t>(meshIndex),
			    static_cast<std::uint32_t>(primitiveIndex),
			    result);
		}
	}
}
