#include "PCH.h"

#include "Gltf/GltfPrimitiveMaterialResolver.h"
#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <format>

ImportedMaterialIndex GltfPrimitiveMaterialResolver::Resolve(
    const cgltf_primitive& primitive,
    const cgltf_data* data,
    std::string_view primitiveLabel,
    SourceImportOutput& output)
{
	if (!primitive.material || output.scene.materials.empty())
	{
		return kInvalidImportedMaterialIndex;
	}

	const std::uint32_t materialIndex = static_cast<std::uint32_t>(primitive.material - data->materials);
	if (materialIndex < output.scene.materials.size())
	{
		return materialIndex;
	}

	throw Diagnostics::Error(std::format("glTF {} references unknown material index {}.", primitiveLabel, materialIndex));
}
