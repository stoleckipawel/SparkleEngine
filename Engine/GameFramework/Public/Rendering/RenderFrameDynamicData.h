#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/Rendering/RenderAssetHandles.h"
#include "GameFramework/Public/Rendering/RenderFrameMetadata.h"
#include "GameFramework/Public/Rendering/RenderObjectId.h"
#include "GameFramework/Public/Scene/Lighting/SceneLightDesc.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <vector>

struct RenderCameraData final
{
	DirectX::XMFLOAT3 Position{0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 Direction{0.0f, 0.0f, 1.0f};
	float FovYDegrees = 60.0f;
	float AspectRatio = 1.0f;
	float NearZ = 0.1f;
	float FarZ = 1000.0f;
};

struct RenderObjectDynamicData final
{
	RenderObjectId Object;
	DirectX::XMFLOAT4X4 WorldMatrix{};
	DirectX::XMFLOAT3X4 WorldInverseTranspose{};
	bool Visible = true;
};

struct RenderJointMatrixRange final
{
	RenderObjectId Object;
	RenderSkeletonAssetHandle Skeleton;
	RenderAnimationAssetHandle Animation;
	std::uint32_t JointMatrixOffset = 0;
	std::uint32_t JointMatrixCount = 0;
};

struct RenderMorphWeightRange final
{
	RenderObjectId Object;
	RenderAnimationAssetHandle Animation;
	std::uint32_t TargetNodeIndex = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t WeightOffset = 0;
	std::uint32_t WeightCount = 0;
};

struct RenderLightData final
{
	RenderObjectId Object;
	SceneLightDesc Description;
};

struct RenderFrameDynamicData final
{
	RenderFrameMetadata Metadata;
	RenderCameraData Camera;
	std::vector<RenderObjectDynamicData> Objects;
	std::vector<RenderLightData> Lights;
	std::vector<RenderJointMatrixRange> JointMatrixRanges;
	std::vector<DirectX::XMFLOAT4X4> JointMatrices;
	std::vector<RenderMorphWeightRange> MorphWeightRanges;
	std::vector<float> MorphWeights;
};
