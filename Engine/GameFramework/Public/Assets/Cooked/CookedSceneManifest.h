#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedSceneManifestMagic = MakeCookedAssetMagic('S', 'S', 'C', 'N');
	inline constexpr std::uint32_t kCookedSceneManifestVersion = 1;
	inline constexpr std::uint32_t kInvalidCookedMaterialAssetIndex = (std::numeric_limits<std::uint32_t>::max)();

	struct SPARKLE_ENGINE_API CookedSceneMeshAssetRef
	{
		CookedAssetId meshAssetId = InvalidCookedAssetId;
	};

	struct SPARKLE_ENGINE_API CookedSceneMaterialAssetRef
	{
		CookedAssetId materialAssetId = InvalidCookedAssetId;
	};

	struct SPARKLE_ENGINE_API CookedSceneInstanceRecord
	{
		std::uint32_t meshAssetIndex = 0;
		std::uint32_t materialAssetIndex = 0;
		DirectX::XMFLOAT4X4 worldTransform = {};
	};

	struct SPARKLE_ENGINE_API CookedSceneManifestHeader
	{
		CookedAssetHeader fileHeader{kCookedSceneManifestMagic, kCookedSceneManifestVersion};
		std::uint32_t meshAssetReferenceCount = 0;
		std::uint32_t materialAssetReferenceCount = 0;
		std::uint32_t instanceCount = 0;
		std::uint32_t reserved = 0;
	};
}

static_assert(
    std::is_trivially_copyable_v<Assets::CookedSceneMeshAssetRef>,
    "CookedSceneMeshAssetRef must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedSceneMaterialAssetRef>,
    "CookedSceneMaterialAssetRef must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedSceneInstanceRecord>,
    "CookedSceneInstanceRecord must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedSceneManifestHeader>,
    "CookedSceneManifestHeader must stay trivially copyable.");