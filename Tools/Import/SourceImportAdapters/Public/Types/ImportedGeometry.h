#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "ImportedSceneIndices.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

enum class ImportedMeshInstanceGroupKind : std::uint32_t
{
	None = 0,
	SharedMeshReference = 1,
	AuthoredInstanceGroup = 2,
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
	DirectX::XMFLOAT4X4 worldTransform = MathUtils::IdentityFloat4x4();
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
