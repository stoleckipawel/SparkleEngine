#pragma once

#include <cstdint>
#include <string_view>

enum class AssetType : uint8_t
{
	Shader,
	ShaderSymbols,
	Texture,
	Mesh,
	Material,
	Scene,
	Audio,
	Font,

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
		case AssetType::Material:
			return "Materials";
		case AssetType::Scene:
			return "Scenes";
		case AssetType::Audio:
			return "Audio";
		case AssetType::Font:
			return "Fonts";
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
		case AssetType::Material:
			return "Material";
		case AssetType::Scene:
			return "Scene";
		case AssetType::Audio:
			return "Audio";
		case AssetType::Font:
			return "Font";
		case AssetType::Count:
		default:
			return "Unknown";
	}
}
