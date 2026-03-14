#pragma once

#include <cstdint>
#include <string_view>

enum class AssetType : uint8_t
{
	Shader,
	ShaderSymbols,
	Texture,
	Mesh,

	Count
};

constexpr std::string_view GetAssetSubdirectory(AssetType type) noexcept
{
	switch (type)
	{
		case AssetType::Shader:
			return "Shaders";
		case AssetType::ShaderSymbols:
			return "Shaders/ShaderSymbols";
		case AssetType::Texture:
			return "Textures";
		case AssetType::Mesh:
			return "Meshes";
		case AssetType::Count:
		default:
			return {};
	}
}

constexpr std::string_view GetAssetTypeName(AssetType type) noexcept
{
	switch (type)
	{
		case AssetType::Shader:
			return "Shader";
		case AssetType::ShaderSymbols:
			return "ShaderSymbols";
		case AssetType::Texture:
			return "Texture";
		case AssetType::Mesh:
			return "Mesh";
		case AssetType::Count:
		default:
			return "Unknown";
	}
}
