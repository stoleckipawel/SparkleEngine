#include "PCH.h"

#include "Gltf/GltfMaterialFeatureDiagnostics.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"

#include <cgltf.h>

void GltfMaterialFeatureDiagnostics::ReportUnsupportedFeatures(
    const cgltf_material& material,
    ImportedMaterialIndex materialIndex,
    SourceImportResult& result)
{
	std::string unsupportedFeatures;
	auto appendFeature = [&unsupportedFeatures](std::string_view featureName)
	{
		if (!unsupportedFeatures.empty())
		{
			unsupportedFeatures += ", ";
		}

		unsupportedFeatures += featureName;
	};

	if (material.has_pbr_specular_glossiness)
	{
		appendFeature("KHR_materials_pbrSpecularGlossiness");
	}
	if (material.unlit)
	{
		appendFeature("KHR_materials_unlit");
	}
	if (material.has_clearcoat)
	{
		appendFeature("KHR_materials_clearcoat");
	}
	if (material.has_transmission)
	{
		appendFeature("KHR_materials_transmission");
	}
	if (material.has_volume)
	{
		appendFeature("KHR_materials_volume");
	}
	if (material.has_specular)
	{
		appendFeature("KHR_materials_specular");
	}
	if (material.has_sheen)
	{
		appendFeature("KHR_materials_sheen");
	}
	if (material.has_iridescence)
	{
		appendFeature("KHR_materials_iridescence");
	}
	if (material.has_diffuse_transmission)
	{
		appendFeature("KHR_materials_diffuse_transmission");
	}
	if (material.has_anisotropy)
	{
		appendFeature("KHR_materials_anisotropy");
	}
	if (material.has_dispersion)
	{
		appendFeature("KHR_materials_dispersion");
	}

	if (!unsupportedFeatures.empty())
	{
		GltfImportDiagnosticLog::ReportUnsupportedMaterialFeatures(materialIndex, unsupportedFeatures, result);
	}
}
