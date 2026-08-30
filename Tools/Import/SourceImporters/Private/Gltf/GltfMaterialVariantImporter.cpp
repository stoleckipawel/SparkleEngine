#include "PCH.h"

#include "Gltf/GltfMaterialVariantImporter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <utility>

class GltfMaterialVariantTranslation final
{
public:
	static ImportedMaterialIndex ResolveImportedMaterialIndex(const cgltf_data* data, const cgltf_material* material) noexcept
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

	static void ImportVariantSet(const cgltf_data* data, SourceImportOutput& output)
	{
		output.scene.materialVariants.reserve(data->variants_count);
		for (cgltf_size variantIndex = 0; variantIndex < data->variants_count; ++variantIndex)
		{
			if (data->variants[variantIndex].name == nullptr || data->variants[variantIndex].name[0] == '\0')
			{
				throw Diagnostics::Error(std::format("glTF material variant {} has no name.", variantIndex));
			}
			ImportedMaterialVariant importedVariant;
			importedVariant.name = data->variants[variantIndex].name;
			importedVariant.sourceVariantIndex = static_cast<std::uint32_t>(variantIndex);
			output.scene.materialVariants.push_back(std::move(importedVariant));
		}
	}

	static void ImportPrimitiveMappings(
	    const cgltf_data* data,
	    const cgltf_primitive& primitive,
	    std::uint32_t sourceMeshIndex,
	    std::uint32_t sourcePrimitiveIndex,
	    SourceImportOutput& output)
	{
		for (cgltf_size mappingIndex = 0; mappingIndex < primitive.mappings_count; ++mappingIndex)
		{
			const cgltf_material_mapping& mapping = primitive.mappings[mappingIndex];
			if (mapping.material == nullptr)
			{
				throw Diagnostics::Error(
				    std::format(
				        "glTF mesh {} primitive {} material-variant mapping {} has no material.",
				        sourceMeshIndex,
				        sourcePrimitiveIndex,
				        mappingIndex));
			}

			const ImportedMaterialIndex materialIndex = ResolveImportedMaterialIndex(data, mapping.material);
			if (materialIndex == kInvalidImportedMaterialIndex || materialIndex >= output.scene.materials.size())
			{
				throw Diagnostics::Error(
				    std::format(
				        "glTF mesh {} primitive {} material-variant mapping {} references an unknown material.",
				        sourceMeshIndex,
				        sourcePrimitiveIndex,
				        mappingIndex));
			}

			const cgltf_size sourceVariantIndex = mapping.variant;
			if (sourceVariantIndex >= output.scene.materialVariants.size())
			{
				throw Diagnostics::Error(
				    std::format(
				        "glTF mesh {} primitive {} material-variant mapping {} references an unknown variant.",
				        sourceMeshIndex,
				        sourcePrimitiveIndex,
				        mappingIndex));
			}

			ImportedMaterialVariantMapping importedMapping;
			importedMapping.sourceMeshIndex = sourceMeshIndex;
			importedMapping.sourcePrimitiveIndex = sourcePrimitiveIndex;
			importedMapping.variantIndex = static_cast<ImportedMaterialVariantIndex>(sourceVariantIndex);
			importedMapping.materialIndex = materialIndex;
			output.scene.materialVariantMappings.push_back(importedMapping);
		}
	}
};

void GltfMaterialVariantImporter::ImportMaterialVariants(const cgltf_data* data, SourceImportOutput& output)
{
	if (data == nullptr || data->variants_count == 0)
	{
		return;
	}

	GltfMaterialVariantTranslation::ImportVariantSet(data, output);
	for (cgltf_size meshIndex = 0; meshIndex < data->meshes_count; ++meshIndex)
	{
		const cgltf_mesh& mesh = data->meshes[meshIndex];
		for (cgltf_size primitiveIndex = 0; primitiveIndex < mesh.primitives_count; ++primitiveIndex)
		{
			GltfMaterialVariantTranslation::ImportPrimitiveMappings(
			    data,
			    mesh.primitives[primitiveIndex],
			    static_cast<std::uint32_t>(meshIndex),
			    static_cast<std::uint32_t>(primitiveIndex),
			    output);
		}
	}
}
