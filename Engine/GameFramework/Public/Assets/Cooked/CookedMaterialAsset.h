#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/Cooked/CookedTextureReference.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

namespace Engine::Assets
{
	enum class CookedAlphaMode : std::uint32_t
	{
		Opaque = 0,
		Mask = 1,
		Blend = 2,
	};

	inline constexpr std::uint32_t kCookedMaterialAssetMagic = MakeCookedAssetMagic('S', 'M', 'A', 'T');
	inline constexpr std::uint32_t kCookedMaterialAssetVersion = 1;

	struct SPARKLE_ENGINE_API CookedMaterialAssetHeader
	{
		CookedAssetHeader fileHeader{kCookedMaterialAssetMagic, kCookedMaterialAssetVersion};
		std::uint32_t nameByteCount = 0;
		std::uint32_t textureReferenceCount = 0;
		std::uint32_t textureReferenceVersion = kCookedTextureReferenceVersion;
		CookedAlphaMode alphaMode = CookedAlphaMode::Opaque;
		DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
		float metallic = 0.0f;
		float roughness = 0.5f;
		float f0 = 0.04f;
		float alphaCutoff = 0.5f;
		DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
		float reserved = 0.0f;
	};
}

static_assert(
	std::is_trivially_copyable_v<Engine::Assets::CookedMaterialAssetHeader>,
	"CookedMaterialAssetHeader must stay trivially copyable.");