#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Core/Public/CoreTypes.h"

#include <DirectXMath.h>
#include <vector>

struct VertexData
{
	DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT2 uv = {0.0f, 0.0f};
	DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT3 normal = {0.0f, 1.0f, 0.0f};
	DirectX::XMFLOAT4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};

	constexpr VertexData() noexcept = default;

	constexpr VertexData(
	    const DirectX::XMFLOAT3& pos,
	    const DirectX::XMFLOAT2& tex,
	    const DirectX::XMFLOAT4& col,
	    const DirectX::XMFLOAT3& norm,
	    const DirectX::XMFLOAT4& tan) noexcept :
	    position(pos), uv(tex), color(col), normal(norm), tangent(tan)
	{
	}

	constexpr explicit VertexData(const DirectX::XMFLOAT3& pos) noexcept : position(pos) {}
};

static_assert(std::is_trivially_copyable_v<VertexData>, "VertexData must be trivially copyable for GPU upload");

struct MeshData
{
	std::vector<VertexData> vertices;
	std::vector<uint32> indices;

	bool IsValid() const noexcept { return !vertices.empty() && !indices.empty(); }

	uint32 GetVertexCount() const noexcept { return static_cast<uint32>(vertices.size()); }
	uint32 GetIndexCount() const noexcept { return static_cast<uint32>(indices.size()); }

	SizeType GetVertexBufferSize() const noexcept { return vertices.size() * sizeof(VertexData); }
	SizeType GetIndexBufferSize() const noexcept { return indices.size() * sizeof(uint32); }

	const VertexData* GetVertexData() const noexcept { return vertices.data(); }
	const uint32* GetIndexData() const noexcept { return indices.data(); }

	void Clear() noexcept
	{
		vertices.clear();
		indices.clear();
	}

	void Reserve(uint32 vertexCount, uint32 indexCount)
	{
		vertices.reserve(vertexCount);
		indices.reserve(indexCount);
	}
};
