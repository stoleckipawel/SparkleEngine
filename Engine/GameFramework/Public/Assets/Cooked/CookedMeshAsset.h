#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedMeshAssetMagic = 0x48534D53u;

	enum class CookedMeshAssetKind : std::uint32_t
	{
		Static = 0,
		Skeletal = 1,
	};

	struct SPARKLE_ENGINE_API CookedMeshSkinInfluence
	{
		std::uint16_t jointIndices[8] = {};
		float jointWeights[8] = {};
	};

	struct SPARKLE_ENGINE_API CookedMeshVertex
	{
		DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT2 uv = {0.0f, 0.0f};
		DirectX::XMFLOAT4 color = {1.0f, 1.0f, 1.0f, 1.0f};
		DirectX::XMFLOAT3 normal = {0.0f, 1.0f, 0.0f};
		DirectX::XMFLOAT4 tangent = {1.0f, 0.0f, 0.0f, 1.0f};
	};

	inline constexpr std::uint32_t kCookedMeshMorphTargetNameCapacity = 64;

	struct SPARKLE_ENGINE_API CookedMeshMorphTargetRecord
	{
		char name[kCookedMeshMorphTargetNameCapacity] = {};
		float defaultWeight = 0.0f;
		std::uint32_t firstDelta = 0;
		std::uint32_t deltaCount = 0;
	};

	struct SPARKLE_ENGINE_API CookedMeshMorphTargetDelta
	{
		DirectX::XMFLOAT3 position = {0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT3 normal = {0.0f, 0.0f, 0.0f};
		DirectX::XMFLOAT3 tangent = {0.0f, 0.0f, 0.0f};
	};

	struct SPARKLE_ENGINE_API CookedMeshAssetHeader
	{
		CookedAssetHeader fileHeader{kCookedMeshAssetMagic};
		std::uint32_t vertexCount = 0;
		std::uint32_t indexCount = 0;
		std::uint32_t skinInfluenceCount = 0;
		std::uint32_t vertexStride = sizeof(CookedMeshVertex);
		std::uint32_t indexStride = sizeof(std::uint32_t);
		std::uint32_t skinInfluenceStride = sizeof(CookedMeshSkinInfluence);
		std::uint32_t morphTargetCount = 0;
		std::uint32_t morphTargetDeltaCount = 0;
		std::uint32_t morphTargetRecordStride = sizeof(CookedMeshMorphTargetRecord);
		std::uint32_t morphTargetDeltaStride = sizeof(CookedMeshMorphTargetDelta);
		std::uint32_t flags = 0;
		CookedMeshAssetKind assetKind = CookedMeshAssetKind::Static;
	};

	enum CookedMeshAssetFlags : std::uint32_t
	{
		CookedMeshAssetFlag_HasSkinInfluences = 1u << 0u,
		CookedMeshAssetFlag_HasMorphTargets = 1u << 1u,
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedMeshSkinInfluence>, "CookedMeshSkinInfluence must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedMeshVertex>, "CookedMeshVertex must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedMeshMorphTargetRecord>,
    "CookedMeshMorphTargetRecord must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedMeshMorphTargetDelta>, "CookedMeshMorphTargetDelta must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedMeshAssetHeader>, "CookedMeshAssetHeader must stay trivially copyable.");
