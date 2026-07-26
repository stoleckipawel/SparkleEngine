#pragma once

#include <DirectXMath.h>

#include <type_traits>

struct MorphTargetDeltaData final
{
	DirectX::XMFLOAT4 Position = {};
	DirectX::XMFLOAT4 Normal = {};
	DirectX::XMFLOAT4 Tangent = {};
};

static_assert(std::is_standard_layout_v<MorphTargetDeltaData>, "MorphTargetDeltaData must be standard-layout");
static_assert(std::is_trivially_copyable_v<MorphTargetDeltaData>, "MorphTargetDeltaData must be trivially copyable");
static_assert(sizeof(MorphTargetDeltaData) == 48, "MorphTargetDeltaData must match the HLSL structured-buffer stride");
