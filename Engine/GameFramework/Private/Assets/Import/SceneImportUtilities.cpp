#include "PCH.h"

#include "Assets/Import/SceneImportUtilities.h"

#include "FileSystemUtils.h"

#include <format>

MaterialDesc SceneImportUtilities::CreateMaterialDesc(std::string name)
{
	MaterialDesc materialDesc;
	materialDesc.name = std::move(name);
	return materialDesc;
}

void SceneImportUtilities::SetMaterialTexture(
    MaterialDesc& materialDesc,
    ImportedTextureSemantic semantic,
    const std::optional<std::filesystem::path>& texturePath)
{
	if (!texturePath)
	{
		return;
	}

	switch (semantic)
	{
		case ImportedTextureSemantic::Albedo:
			materialDesc.albedoTexture = *texturePath;
			break;
		case ImportedTextureSemantic::Normal:
			materialDesc.normalTexture = *texturePath;
			break;
		case ImportedTextureSemantic::MetallicRoughness:
			materialDesc.metallicRoughnessTexture = *texturePath;
			break;
		case ImportedTextureSemantic::Occlusion:
			materialDesc.occlusionTexture = *texturePath;
			break;
		case ImportedTextureSemantic::Emissive:
			materialDesc.emissiveTexture = *texturePath;
			break;
	}
}

std::optional<std::filesystem::path> SceneImportUtilities::NormalizeImportedTexturePath(
    const std::filesystem::path& sourceDirectory,
    const std::filesystem::path& importedTexturePath)
{
	if (importedTexturePath.empty())
	{
		return std::nullopt;
	}

	std::filesystem::path resolvedTexturePath = importedTexturePath;
	if (!resolvedTexturePath.is_absolute())
	{
		resolvedTexturePath = sourceDirectory / resolvedTexturePath;
	}

	resolvedTexturePath = Filesystem::NormalizePath(resolvedTexturePath);
	if (resolvedTexturePath.empty())
	{
		return std::nullopt;
	}

	return resolvedTexturePath;
}

std::uint32_t SceneImportUtilities::SanitizeMaterialOffset(
    std::uint32_t requestedOffset,
    std::size_t materialCount,
    std::string_view importerName,
    std::string_view meshLabel,
    SceneImportResult& result)
{
	if (materialCount == 0)
	{
		return 0;
	}

	if (requestedOffset < materialCount)
	{
		return requestedOffset;
	}

	result.AddWarning(
	    std::format(
	        "{}: '{}' references invalid material index {} and will use the default material",
	        importerName,
	        meshLabel,
	        requestedOffset));
	return 0;
}

Transform SceneImportUtilities::BuildImportedTransform(const DirectX::XMMATRIX& worldTransform) noexcept
{
	return Transform(worldTransform);
}