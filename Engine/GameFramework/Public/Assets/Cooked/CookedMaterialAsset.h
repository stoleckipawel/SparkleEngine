#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/Cooked/CookedTextureReference.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

namespace Assets
{
	enum class CookedAlphaMode : std::uint32_t
	{
		Opaque = 0,
		Mask = 1,
		Blend = 2,
	};

	inline constexpr std::uint32_t kCookedMaterialAssetMagic = 0x54414D53u;

	struct SPARKLE_ENGINE_API CookedMaterialAssetHeader
	{
		CookedAssetHeader fileHeader{kCookedMaterialAssetMagic};
		std::uint32_t nameByteCount = 0;
		std::uint32_t textureReferenceCount = 0;
		CookedAlphaMode alphaMode = CookedAlphaMode::Opaque;
		DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
		float metallic = 0.0f;
		float roughness = 0.5f;
		float f0 = 0.04f;
		DirectX::XMFLOAT3 subsurfaceColor = {0.0f, 0.0f, 0.0f};
		float subsurfaceStrength = 0.0f;
		float alphaCutoff = 0.5f;
		DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
		std::uint32_t doubleSided = 0;
	};
}

static_assert(std::is_trivially_copyable_v<Assets::CookedMaterialAssetHeader>, "CookedMaterialAssetHeader must stay trivially copyable.");
