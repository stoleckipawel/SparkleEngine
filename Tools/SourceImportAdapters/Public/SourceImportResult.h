#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "Core/Public/Assets/TextureProperties.h"

#include <DirectXMath.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
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

enum class ImportedAlphaMode : std::uint32_t
{
	Opaque = 0,
	Mask = 1,
	Blend = 2,
};

using ImportedMaterialIndex = std::uint32_t;

constexpr ImportedMaterialIndex kInvalidImportedMaterialIndex = (std::numeric_limits<ImportedMaterialIndex>::max)();

struct ImportedTextureSource
{
	TextureGroup textureGroup = TextureGroup::Default;
	std::filesystem::path sourcePath;
	TextureChannelMask channelMask = TextureChannelMask::Rgba;
};

struct ImportedVertex
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT2 uv = {0.0f, 0.0f};
	DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT3 normal = {0.0f, 1.0f, 0.0f};
	DirectX::XMFLOAT4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
};

static_assert(std::is_trivially_copyable_v<ImportedVertex>, "ImportedVertex must be trivially copyable for mesh cooking");

struct ImportedMeshGeometry
{
	std::vector<ImportedVertex> vertices;
	std::vector<std::uint32_t> indices;

	bool IsValid() const noexcept { return !vertices.empty() && !indices.empty(); }

	void Reserve(std::uint32_t vertexCount, std::uint32_t indexCount)
	{
		vertices.reserve(vertexCount);
		indices.reserve(indexCount);
	}
};

struct ImportedMesh
{
	ImportedMeshGeometry geometry;
	std::string displayName;
	DirectX::XMFLOAT4X4 worldTransform = {
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f};
	ImportedMaterialIndex materialIndex = kInvalidImportedMaterialIndex;

	bool HasMaterialBinding() const noexcept { return materialIndex != kInvalidImportedMaterialIndex; }
};

struct ImportedMaterial
{
	std::string name;

	DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float metallic = 0.0f;
	float roughness = 0.5f;
	float f0 = 0.04f;
	DirectX::XMFLOAT3 subsurfaceColor = {0.0f, 0.0f, 0.0f};
	float subsurfaceStrength = 0.0f;
	DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
	ImportedAlphaMode alphaMode = ImportedAlphaMode::Opaque;
	float alphaCutoff = 0.5f;

	std::vector<ImportedTextureSource> textureSources;
};

struct ImportedScene
{
	std::vector<ImportedMesh> meshes;
	std::vector<ImportedMaterial> materials;
	std::filesystem::path sourcePath;
	SourceImporterType importerType = SourceImporterType::None;

	std::size_t GetMeshCount() const noexcept { return meshes.size(); }
	std::size_t GetMaterialCount() const noexcept { return materials.size(); }

	void ReserveMeshes(std::size_t meshCount)
	{
		meshes.reserve(meshCount);
	}
};

struct SourceImportResult
{
	ImportedScene scene;
	bool succeeded = false;

	bool IsValid() const noexcept { return succeeded && !scene.meshes.empty(); }
	std::size_t GetMeshCount() const noexcept { return scene.GetMeshCount(); }
	std::size_t GetMaterialCount() const noexcept { return scene.GetMaterialCount(); }

	void ReserveMeshes(std::size_t meshCount)
	{
		scene.ReserveMeshes(meshCount);
	}
};


