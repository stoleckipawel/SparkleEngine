#include "PCH.h"

#include "Gltf/GltfPrimitiveMaterialResolver.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"

#include <cgltf.h>

ImportedMaterialIndex GltfPrimitiveMaterialResolver::Resolve(
    const cgltf_primitive& primitive,
    const cgltf_data* data,
    std::string_view primitiveLabel,
    SourceImportResult& result)
{
	if (!primitive.material || result.scene.materials.empty())
	{
		return kInvalidImportedMaterialIndex;
	}

	const std::uint32_t materialIndex = static_cast<std::uint32_t>(primitive.material - data->materials);
	if (materialIndex < result.scene.materials.size())
	{
		return materialIndex;
	}

	GltfImportDiagnosticLog::ReportInvalidMaterialIndex(primitiveLabel, materialIndex, result);
	return kInvalidImportedMaterialIndex;
}
