#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightKind.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshKind.h"

#include <DirectXMath.h>
#include <compare>
#include <cstdint>
#include <limits>

namespace ECS
{
	struct MeshResourceHandle final
	{
		std::uint32_t Slot = (std::numeric_limits<std::uint32_t>::max)();
		std::uint32_t Generation = 0;

		constexpr bool IsValid() const noexcept
		{
			return Slot != (std::numeric_limits<std::uint32_t>::max)() && Generation != 0;
		}
		constexpr auto operator<=>(const MeshResourceHandle&) const noexcept = default;
	};

	struct MeshInstance final
	{
		MeshResourceHandle Resource;
		Assets::CookedAssetId MeshAssetId = Assets::InvalidCookedAssetId;
		Assets::CookedAssetId SkeletonAssetId = Assets::InvalidCookedAssetId;
		MaterialHandle Material = MaterialHandle::Invalid();
		SceneMeshKind Kind = SceneMeshKind::Static;
		SceneMeshAssetIndex MeshAssetIndex = kInvalidSceneMeshAssetIndex;
		SceneMeshInstanceGroupIndex InstanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex;
		std::uint32_t SourceNodeIndex = Assets::kInvalidCookedSceneSourceNodeIndex;
	};

	struct Visibility final
	{
		std::uint32_t LayerMask = (std::numeric_limits<std::uint32_t>::max)();
		bool Visible = true;
	};

	struct Camera final
	{
		float VerticalFieldOfViewDegrees = 60.0f;
		float NearPlane = 0.1f;
		float FarPlane = 1000.0f;
		float AspectRatio = 1.0f;
		CameraProjectionKind ProjectionKind = CameraProjectionKind::Perspective;
		bool Active = true;
	};

	struct CameraMovement final
	{
		float MoveSpeed = 0.10f;
		float MinimumMoveSpeed = 0.0001f;
		float MaximumMoveSpeed = 10.0f;
		float SpeedStep = 0.0001f;
		float SprintMultiplier = 2.0f;
		float MouseSensitivity = 0.00125f;
		bool InvertY = false;
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
		DirectX::XMFLOAT3 Direction{0.0f, -1.0f, 0.0f};
		DirectX::XMFLOAT3 Tangent{1.0f, 0.0f, 0.0f};
		float SourceRadius = 0.05f;
		float AngularDiameterRadians = 0.009308f;
		bool CastShadow = true;
	};
}
