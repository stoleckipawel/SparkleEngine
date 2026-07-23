#include "PCH.h"

#include "Gltf/GltfMaterialImporter.h"

#include "Gltf/GltfMaterialPropertyMapper.h"
#include "Gltf/GltfMaterialTextureMapper.h"

#include <cgltf.h>

#include <format>

static const auto g_gltfMaterialImporterLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Gltf");

class GltfMaterialImporterOperations final
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

	static void WarnUnsupportedFeatures(const cgltf_material& material, ImportedMaterialIndex materialIndex)
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
		if (material.has_transmission)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_transmission");
		}
		if (material.has_volume)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_volume");
		}
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
		if (material.has_diffuse_transmission)
		{
			AppendFeatureName(unsupportedFeatures, "KHR_materials_diffuse_transmission");
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
			SPDLOG_LOGGER_WARN(
			    g_gltfMaterialImporterLogger,
			    "{}",
			    std::format(
			        "GltfImporter: Material handle {} uses unsupported glTF material features [{}] and will be approximated with Sparkle PBR defaults",
			        materialIndex,
			        unsupportedFeatures));
		}
	}
};

void GltfMaterialImporter::ImportMaterials(const cgltf_data* data, const std::filesystem::path& sourceDirectory, SourceImportResult& result)
{
	for (cgltf_size materialIndex = 0; materialIndex < data->materials_count; ++materialIndex)
	{
		result.scene.materials.push_back(ExtractMaterial(
		    data->materials[materialIndex],
		    static_cast<ImportedMaterialIndex>(materialIndex),
		    sourceDirectory,
		    result));
	}
}

ImportedMaterial GltfMaterialImporter::ExtractMaterial(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    const std::filesystem::path& sourceDirectory,
    SourceImportResult& result)
{
	ImportedMaterial importedMaterial;
	GltfMaterialImporterOperations::WarnUnsupportedFeatures(material, materialIndex);
	GltfMaterialPropertyMapper::Apply(material, importedMaterial);
	GltfMaterialTextureMapper::Apply(material, materialIndex, sourceDirectory, importedMaterial, result);
	return importedMaterial;
}


