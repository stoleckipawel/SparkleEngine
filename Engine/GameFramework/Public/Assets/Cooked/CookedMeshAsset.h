#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedMeshAssetMagic = MakeCookedAssetMagic('S', 'M', 'S', 'H');
	inline constexpr std::uint32_t kCookedMeshAssetVersion = 4;

	enum class CookedMeshAssetKind : std::uint32_t
	{
		Static = 0,
		Skeletal = 1,
	};

	struct SPARKLE_ENGINE_API CookedMeshSkinInfluence
	{
		std::uint16_t jointIndices[4] = {0, 0, 0, 0};
		float jointWeights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	};

	struct SPARKLE_ENGINE_API CookedMeshVertex
	{
		DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT2 uv = {0.0f, 0.0f};
		DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		DirectX::XMFLOAT3 normal = {0.0f, 1.0f, 0.0f};
		DirectX::XMFLOAT4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
	};

	struct SPARKLE_ENGINE_API CookedMeshAssetHeader
	{
		CookedAssetHeader fileHeader{kCookedMeshAssetMagic, kCookedMeshAssetVersion};
		std::uint32_t vertexCount = 0;
		std::uint32_t indexCount = 0;
		std::uint32_t skinInfluenceCount = 0;
		std::uint32_t vertexStride = sizeof(CookedMeshVertex);
		std::uint32_t indexStride = sizeof(std::uint32_t);
		std::uint32_t skinInfluenceStride = sizeof(CookedMeshSkinInfluence);
		std::uint32_t flags = 0;
		CookedMeshAssetKind assetKind = CookedMeshAssetKind::Static;
	};

	enum CookedMeshAssetFlags : std::uint32_t
	{
		CookedMeshAssetFlag_HasSkinInfluences = 1u << 0u,
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedMeshSkinInfluence>, "CookedMeshSkinInfluence must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedMeshVertex>, "CookedMeshVertex must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedMeshAssetHeader>, "CookedMeshAssetHeader must stay trivially copyable.");
