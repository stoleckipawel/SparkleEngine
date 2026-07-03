#include "PCH.h"

#include "Gltf/GltfPrimitiveMaterialResolver.h"

#include <cgltf.h>

#include <format>

static const auto g_gltfPrimitiveMaterialResolverLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Gltf");

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

	(void)result;
	SPDLOG_LOGGER_WARN(
	    g_gltfPrimitiveMaterialResolverLogger,
	    "{}",
	    std::format(
	        "GltfImporter: {} references invalid material index {} and will use the default material",
	        primitiveLabel,
	        materialIndex));
	return kInvalidImportedMaterialIndex;
}
