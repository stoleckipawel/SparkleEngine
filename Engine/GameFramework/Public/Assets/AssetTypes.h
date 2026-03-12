// ============================================================================
// AssetTypes.h
// Classification of engine asset types with directory mapping.
// ----------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string_view>

enum class AssetType : uint8_t
{
	Shader,         // HLSL source files (.hlsl, .hlsli)
	ShaderSymbols,  // Compiled shader debug symbols (.pdb)
	Texture,        // Image files (.png, .jpg, .dds, etc.)
	Mesh,           // 3D model files (.gltf, .glb, .obj, etc.)
	Material,       // Material definitions (.mat, .json)
	Scene,          // Scene/level files (.scene, .json)
	Audio,          // Sound files (.wav, .ogg, .mp3)
	Font,           // Font files (.ttf, .otf)

	Count
};

// Returns the canonical subdirectory name for a given asset type.
// Used by AssetSystem to construct full filesystem paths.
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

// Returns a human-readable name for logging and debugging.
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
