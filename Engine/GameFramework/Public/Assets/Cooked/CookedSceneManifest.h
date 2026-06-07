#pragma once

#include "Core/Public/Math/MathUtils.h"
#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/Cooked/CookedMeshAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneCameraRecord.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneLightRecord.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneMetadataRecords.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <type_traits>

namespace Assets
{
	inline constexpr std::uint32_t kCookedSceneManifestMagic = MakeCookedAssetMagic('S', 'S', 'C', 'N');
	inline constexpr std::uint32_t kCookedSceneManifestVersion = 6;
	inline constexpr std::uint32_t kInvalidCookedMaterialAssetIndex = (std::numeric_limits<std::uint32_t>::max)();
	inline constexpr std::uint32_t kInvalidCookedSceneInstanceGroupIndex = (std::numeric_limits<std::uint32_t>::max)();
	inline constexpr std::uint32_t kInvalidCookedSceneSkeletonRefIndex = (std::numeric_limits<std::uint32_t>::max)();

	enum class CookedSceneInstanceGroupKind : std::uint32_t
	{
		None = 0,
		SharedMeshReference = 1,
		AuthoredInstanceGroup = 2,
	};

	struct SPARKLE_ENGINE_API CookedSceneMeshAssetRef
	{
		CookedAssetId meshAssetId = InvalidCookedAssetId;
		CookedMeshAssetKind meshAssetKind = CookedMeshAssetKind::Static;
	};

	struct SPARKLE_ENGINE_API CookedSceneMaterialAssetRef
	{
		CookedAssetId materialAssetId = InvalidCookedAssetId;
	};

	struct SPARKLE_ENGINE_API CookedSceneInstanceRecord
	{
		std::uint32_t meshAssetIndex = 0;
		std::uint32_t materialAssetIndex = 0;
		std::uint32_t groupIndex = kInvalidCookedSceneInstanceGroupIndex;
		std::uint32_t skeletonRefIndex = kInvalidCookedSceneSkeletonRefIndex;
		DirectX::XMFLOAT4X4 worldTransform = MathUtils::IdentityFloat4x4();
	};

	struct SPARKLE_ENGINE_API CookedSceneInstanceGroupRecord
	{
		std::uint32_t meshAssetIndex = 0;
		std::uint32_t materialAssetIndex = 0;
		std::uint32_t firstInstance = 0;
		std::uint32_t instanceCount = 0;
		CookedSceneInstanceGroupKind groupKind = CookedSceneInstanceGroupKind::None;
		std::uint32_t flags = 0;
	};

	struct SPARKLE_ENGINE_API CookedSceneManifestHeader
	{
		CookedAssetHeader fileHeader{kCookedSceneManifestMagic, kCookedSceneManifestVersion};
		std::uint32_t meshAssetReferenceCount = 0;
		std::uint32_t materialAssetReferenceCount = 0;
		std::uint32_t instanceCount = 0;
		std::uint32_t instanceGroupCount = 0;
		std::uint32_t cameraCount = 0;
		std::uint32_t lightCount = 0;
		std::uint32_t skeletonRefCount = 0;
		std::uint32_t animationRefCount = 0;
		std::uint32_t featureFlags = 0;
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedSceneMeshAssetRef>, "CookedSceneMeshAssetRef must stay trivially copyable.");
static_assert(
    std::is_trivially_copyable_v<Assets::CookedSceneMaterialAssetRef>,
    "CookedSceneMaterialAssetRef must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedSceneInstanceRecord>, "CookedSceneInstanceRecord must stay trivially copyable.");
static_assert(
	std::is_trivially_copyable_v<Assets::CookedSceneInstanceGroupRecord>,
	"CookedSceneInstanceGroupRecord must stay trivially copyable.");
static_assert(std::is_trivially_copyable_v<Assets::CookedSceneManifestHeader>, "CookedSceneManifestHeader must stay trivially copyable.");
