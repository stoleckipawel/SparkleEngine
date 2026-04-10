#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

namespace Engine::Assets
{
	inline constexpr std::uint32_t kCookedMeshAssetMagic = MakeCookedAssetMagic('S', 'M', 'S', 'H');
	inline constexpr std::uint32_t kCookedMeshAssetVersion = 1;

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
		std::uint32_t vertexStride = sizeof(CookedMeshVertex);
		std::uint32_t indexStride = sizeof(std::uint32_t);
	};
}

static_assert(
	std::is_trivially_copyable_v<Engine::Assets::CookedMeshVertex>,
	"CookedMeshVertex must stay trivially copyable.");
static_assert(
	std::is_trivially_copyable_v<Engine::Assets::CookedMeshAssetHeader>,
	"CookedMeshAssetHeader must stay trivially copyable.");