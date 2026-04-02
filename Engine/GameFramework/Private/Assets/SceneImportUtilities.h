#pragma once

#include "Assets/SceneImportResult.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

enum class ImportedTextureSemantic : std::uint8_t
{
	Albedo,
	Normal,
	MetallicRoughness,
	Occlusion,
	Emissive
};

namespace SceneImportUtilities
{
	MaterialDesc CreateMaterialDesc(std::string name);

	void SetMaterialTexture(
	    MaterialDesc& materialDesc,
	    ImportedTextureSemantic semantic,
	    const std::optional<std::filesystem::path>& texturePath);

	std::optional<std::filesystem::path> NormalizeImportedTexturePath(
	    const std::filesystem::path& sourceDirectory,
	    const std::filesystem::path& importedTexturePath);

	std::uint32_t SanitizeMaterialOffset(
	    std::uint32_t requestedOffset,
	    std::size_t materialCount,
	    std::string_view importerName,
	    std::string_view meshLabel,
	    SceneImportResult& result);
        
	Transform BuildImportedTransform(const DirectX::XMMATRIX& worldTransform) noexcept;
}