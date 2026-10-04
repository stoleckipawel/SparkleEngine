#include "PCH.h"

#include "Gltf/GltfMaterialImporter.h"

#include "Gltf/GltfMaterialPropertyMapper.h"
#include "Gltf/GltfMaterialTextureMapper.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Logger.h"

#include <cgltf.h>

#include <format>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(gltfMaterialImporterLogger, "Tools.SourceImporters.GltfMaterials");

class GltfMaterialFeatureReporting final
{
public:
	static void ValidateRequiredExtensions(const cgltf_data& data)
	{
		for (cgltf_size index = 0; index < data.extensions_required_count; ++index)
		{
			const std::string_view extension = data.extensions_required[index];
			if (extension == "KHR_materials_anisotropy" || extension == "KHR_materials_clearcoat"
			    || extension == "KHR_materials_sheen" || extension == "KHR_materials_transmission"
			    || extension == "KHR_materials_volume")
			{
				throw Diagnostics::Error(std::format("Required glTF material extension '{}' has no shading implementation.", extension));
			}
		}
	}

	static void ReportGenericShading(const cgltf_material& material, ImportedMaterialIndex materialIndex)
	{
		std::string omittedFeatures;
		if (material.has_anisotropy)
		{
			AppendFeatureName(omittedFeatures, "anisotropy");
		}
		if (material.has_clearcoat)
		{
			AppendFeatureName(omittedFeatures, "clearcoat");
		}
		if (material.has_sheen)
		{
			AppendFeatureName(omittedFeatures, "sheen");
		}
		if (material.has_transmission)
		{
			AppendFeatureName(omittedFeatures, "transmission");
		}
		if (material.has_volume)
		{
			AppendFeatureName(omittedFeatures, "volume");
		}
		if (!omittedFeatures.empty())
		{
			SPDLOG_LOGGER_WARN(
			    gltfMaterialImporterLogger,
			    "glTF material {} uses generic metallic-roughness shading; omitted optional lobes: {}.",
			    materialIndex,
			    omittedFeatures);
		}
	}

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
		if (material.has_specular)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_specular");
		}
		if (material.has_iridescence)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_iridescence");
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
	GltfMaterialFeatureReporting::ValidateRequiredExtensions(*data);
	for (cgltf_size materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
	{
		output.scene.materials.push_back(
		    ExtractMaterial(data->materials[materialIndex], static_cast<ImportedMaterialIndex>(materialIndex), sourceDirectory));
		GltfMaterialFeatureReporting::ReportGenericShading(data->materials[materialIndex], static_cast<ImportedMaterialIndex>(materialIndex));
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
