#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "Core/Public/Assets/TextureProperties.h"
#include "SourceImportDiagnostics.h"

#include <DirectXMath.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

enum class ImportedAlphaMode : std::uint32_t
{
	Opaque = 0,
	Mask = 1,
	Blend = 2,
};

using ImportedMaterialIndex = std::uint32_t;
using ImportedMeshPrimitiveIndex = std::uint32_t;
using ImportedMeshInstanceIndex = std::uint32_t;
using ImportedMeshInstanceGroupIndex = std::uint32_t;

constexpr ImportedMaterialIndex kInvalidImportedMaterialIndex = (std::numeric_limits<ImportedMaterialIndex>::max)();
constexpr ImportedMeshPrimitiveIndex kInvalidImportedMeshPrimitiveIndex = (std::numeric_limits<ImportedMeshPrimitiveIndex>::max)();
constexpr ImportedMeshInstanceIndex kInvalidImportedMeshInstanceIndex = (std::numeric_limits<ImportedMeshInstanceIndex>::max)();
constexpr ImportedMeshInstanceGroupIndex kInvalidImportedMeshInstanceGroupIndex =
    (std::numeric_limits<ImportedMeshInstanceGroupIndex>::max)();

enum class ImportedMeshInstanceGroupKind : std::uint32_t
{
	None = 0,
	SharedMeshReference = 1,
	AuthoredInstanceGroup = 2,
};

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

struct ImportedMeshPrimitive
{
	ImportedMeshGeometry geometry;
	std::string displayName;
	std::uint32_t sourceMeshIndex = 0;
	std::uint32_t sourcePrimitiveIndex = 0;
};

struct ImportedMeshInstance
{
	ImportedMeshPrimitiveIndex primitiveIndex = kInvalidImportedMeshPrimitiveIndex;
	ImportedMaterialIndex materialIndex = kInvalidImportedMaterialIndex;
	ImportedMeshInstanceGroupIndex groupIndex = kInvalidImportedMeshInstanceGroupIndex;
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
	std::uint32_t sourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::string sourceNodeName;

	bool HasPrimitiveBinding() const noexcept { return primitiveIndex != kInvalidImportedMeshPrimitiveIndex; }
	bool HasMaterialBinding() const noexcept { return materialIndex != kInvalidImportedMaterialIndex; }
};

struct ImportedMeshInstanceGroup
{
	ImportedMeshPrimitiveIndex primitiveIndex = kInvalidImportedMeshPrimitiveIndex;
	ImportedMaterialIndex materialIndex = kInvalidImportedMaterialIndex;
	ImportedMeshInstanceIndex firstInstanceIndex = kInvalidImportedMeshInstanceIndex;
	std::uint32_t instanceCount = 0;
	ImportedMeshInstanceGroupKind groupKind = ImportedMeshInstanceGroupKind::None;
	std::uint32_t flags = 0;

	bool HasPrimitiveBinding() const noexcept { return primitiveIndex != kInvalidImportedMeshPrimitiveIndex; }
	bool HasMaterialBinding() const noexcept { return materialIndex != kInvalidImportedMaterialIndex; }
	bool HasInstanceRange() const noexcept { return firstInstanceIndex != kInvalidImportedMeshInstanceIndex && instanceCount > 0; }
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
	std::vector<ImportedMeshPrimitive> meshPrimitives;
	std::vector<ImportedMeshInstance> meshInstances;
	std::vector<ImportedMeshInstanceGroup> meshInstanceGroups;
	std::vector<ImportedMaterial> materials;
	std::filesystem::path sourcePath;
	std::string importerName = "SourceSceneImporter";

	std::size_t GetMeshCount() const noexcept { return meshInstances.size(); }
	std::size_t GetMeshPrimitiveCount() const noexcept { return meshPrimitives.size(); }
	std::size_t GetMeshInstanceCount() const noexcept { return meshInstances.size(); }
	std::size_t GetMeshInstanceGroupCount() const noexcept { return meshInstanceGroups.size(); }
	std::size_t GetMaterialCount() const noexcept { return materials.size(); }

	void ReserveMeshPrimitives(std::size_t primitiveCount)
	{
		meshPrimitives.reserve(primitiveCount);
	}

	void ReserveMeshInstances(std::size_t instanceCount)
	{
		meshInstances.reserve(instanceCount);
	}

	void ReserveMeshInstanceGroups(std::size_t instanceGroupCount)
	{
		meshInstanceGroups.reserve(instanceGroupCount);
	}
};

struct SourceImportResult
{
	ImportedScene scene;
	SourceImportDiagnostics diagnostics;
	bool succeeded = false;

	bool IsValid() const noexcept { return succeeded && !scene.meshPrimitives.empty() && !scene.meshInstances.empty(); }
	std::size_t GetMeshCount() const noexcept { return scene.GetMeshCount(); }
	std::size_t GetMeshPrimitiveCount() const noexcept { return scene.GetMeshPrimitiveCount(); }
	std::size_t GetMeshInstanceCount() const noexcept { return scene.GetMeshInstanceCount(); }
	std::size_t GetMeshInstanceGroupCount() const noexcept { return scene.GetMeshInstanceGroupCount(); }
	std::size_t GetMaterialCount() const noexcept { return scene.GetMaterialCount(); }
	std::string_view GetImporterName() const noexcept { return scene.importerName; }

	void ReserveMeshPrimitives(std::size_t primitiveCount)
	{
		scene.ReserveMeshPrimitives(primitiveCount);
	}

	void ReserveMeshInstances(std::size_t instanceCount)
	{
		scene.ReserveMeshInstances(instanceCount);
	}

	void ReserveMeshInstanceGroups(std::size_t instanceGroupCount)
	{
		scene.ReserveMeshInstanceGroups(instanceGroupCount);
	}
};


