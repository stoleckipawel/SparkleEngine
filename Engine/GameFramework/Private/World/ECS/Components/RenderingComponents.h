#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightKind.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshKind.h"

#include <DirectXMath.h>
#include <cstdint>
#include <limits>

namespace ECS
{
	struct MeshInstance final
	{
		Assets::CookedAssetId MeshAssetId = Assets::InvalidCookedAssetId;
		Assets::CookedAssetId SkeletonAssetId = Assets::InvalidCookedAssetId;
		MaterialHandle Material = MaterialHandle::Invalid();
		SceneMeshKind Kind = SceneMeshKind::Static;
	};

	struct Visibility final
	{
		std::uint32_t LayerMask = (std::numeric_limits<std::uint32_t>::max)();
		bool Visible = true;
	};

	struct Camera final
	{
		float VerticalFieldOfViewRadians = DirectX::XM_PIDIV4;
		float NearPlane = 0.1f;
		float FarPlane = 1000.0f;
		float AspectRatio = 1.0f;
		bool Active = true;
	};

	struct Light final
	{
		SceneLightKind Kind = SceneLightKind::Unknown;
		DirectX::XMFLOAT3 Color{1.0f, 1.0f, 1.0f};
		float Intensity = 1.0f;
		float Range = 0.0f;
		float InnerConeRadians = 0.0f;
		float OuterConeRadians = 0.0f;
		DirectX::XMFLOAT2 AreaSize{0.0f, 0.0f};
	};
}
