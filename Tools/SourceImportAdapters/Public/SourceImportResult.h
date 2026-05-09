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
	struct MaterialTextureSource
	{
		TextureGroup textureGroup = TextureGroup::Default;
		std::filesystem::path sourcePath;
		TextureChannelMask channelMask = TextureChannelMask::Rgba;
	};

	std::vector<MeshData> meshes;
	std::vector<MaterialDesc> materials;
	std::vector<std::vector<MaterialTextureSource>> materialTextureSources;
	std::vector<Transform> transforms;
	std::vector<MaterialHandle> materialHandles;
	SourceImporterType importerType = SourceImporterType::None;

	bool bSuccess = false;

	bool IsValid() const noexcept { return bSuccess && !meshes.empty(); }
	std::size_t GetMeshCount() const noexcept { return meshes.size(); }
	std::size_t GetMaterialCount() const noexcept { return materials.size(); }

	void Reserve(std::size_t primitiveCount)
	{
		meshes.reserve(primitiveCount);
		transforms.reserve(primitiveCount);
		materialHandles.reserve(primitiveCount);
	}
};


