#include "PCH.h"

#include "Gltf/GltfMaterialImporter.h"

#include "Gltf/GltfMaterialFeatureDiagnostics.h"
#include "Gltf/GltfMaterialPropertyMapper.h"
#include "Gltf/GltfMaterialTextureMapper.h"

#include <cgltf.h>

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
	GltfMaterialFeatureDiagnostics::ReportUnsupportedFeatures(material, materialIndex, result);
	GltfMaterialPropertyMapper::Apply(material, importedMaterial);
	GltfMaterialTextureMapper::Apply(material, materialIndex, sourceDirectory, importedMaterial, result);
	return importedMaterial;
}


