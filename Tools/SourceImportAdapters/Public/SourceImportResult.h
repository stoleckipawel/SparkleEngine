#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "Core/Public/Assets/TextureProperties.h"
#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshData.h"
#include "GameFramework/Public/Scene/Transform.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

enum class SourceImporterType : std::uint8_t
{
	None = 0,
	Fbx,
	Gltf
};

constexpr std::string_view GetSourceImporterTypeName(SourceImporterType importerType) noexcept
{
	switch (importerType)
	{
		case SourceImporterType::Fbx:
			return "FbxImporter";
		case SourceImporterType::Gltf:
			return "GltfImporter";
		case SourceImporterType::None:
		default:
			return "SourceSceneImporter";
	}
}

struct SourceImportResult
{
	struct TextureSource
	{
		TextureGroup textureGroup = TextureGroup::Default;
		std::filesystem::path sourcePath;
		TextureChannelMask channelMask = TextureChannelMask::Rgba;
	};

	struct MeshEntry
	{
		MeshData geometry;
		std::string displayName;
		Transform transform;
		MaterialHandle material;
	};

	struct MaterialEntry
	{
		MaterialDesc description;
		std::vector<TextureSource> textures;
	};

	std::vector<MeshEntry> meshes;
	std::vector<MaterialEntry> materials;
	std::filesystem::path sourceScenePath;
	SourceImporterType importerType = SourceImporterType::None;

	bool succeeded = false;

	bool IsValid() const noexcept { return succeeded && !meshes.empty(); }
	std::size_t GetMeshCount() const noexcept { return meshes.size(); }
	std::size_t GetMaterialCount() const noexcept { return materials.size(); }

	void ReserveMeshes(std::size_t meshCount)
	{
		meshes.reserve(meshCount);
	}
};


