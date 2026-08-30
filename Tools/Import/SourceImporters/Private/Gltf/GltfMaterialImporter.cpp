#include "PCH.h"

#include "Gltf/GltfMaterialImporter.h"

#include "Gltf/GltfMaterialPropertyMapper.h"
#include "Gltf/GltfMaterialTextureMapper.h"
#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <format>

class GltfMaterialFeatureReporting final
{
public:
	static void AppendFeatureName(std::string& unsupportedFeatures, std::string_view featureName)
	{
		if (!unsupportedFeatures.empty())
		{
			unsupportedFeatures += ", ";
		}

		unsupportedFeatures += featureName;
	}

	static void ValidateFeatureSupport(const cgltf_material& material, ImportedMaterialIndex materialIndex)
	{
		std::string unsupportedFeatures;
		if (material.has_pbr_specular_glossiness)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_pbrSpecularGlossiness");
		}
		if (material.unlit)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_unlit");
		}
		if (material.has_clearcoat)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_clearcoat");
		}
		// Product policy intentionally discards optional transmission (including diffuse transmission) and volume data.
		// The imported metallic-roughness material remains authoritative; these extensions do not create a fallback renderer
		// path or runtime fields.
		if (material.has_specular)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_specular");
		}
		if (material.has_sheen)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_sheen");
		}
		if (material.has_iridescence)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_iridescence");
		}
		if (material.has_anisotropy)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_anisotropy");
		}
		if (material.has_dispersion)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_dispersion");
		}

		if (!unsupportedFeatures.empty())
		{
			throw Diagnostics::Error(std::format("glTF material {} uses unsupported features [{}].", materialIndex, unsupportedFeatures));
		}
	}
};

void GltfMaterialImporter::ImportMaterials(const cgltf_data* data, const std::filesystem::path& sourceDirectory, SourceImportOutput& output)
{
	for (cgltf_size materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
	{
		output.scene.materials.push_back(
		    ExtractMaterial(data->materials[materialIndex], static_cast<ImportedMaterialIndex>(materialIndex), sourceDirectory));
	}
}

ImportedMaterial GltfMaterialImporter::ExtractMaterial(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory)
{
	ImportedMaterial importedMaterial;
	GltfMaterialFeatureReporting::ValidateFeatureSupport(material, materialIndex);
	GltfMaterialPropertyMapper::Apply(material, importedMaterial);
	GltfMaterialTextureMapper::Apply(material, materialIndex, sourceDirectory, importedMaterial);
	return importedMaterial;
}
