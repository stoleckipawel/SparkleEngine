#pragma once

#include <DirectXMath.h>

#include <cstddef>

struct ShadowConstantBufferData
{
	DirectX::XMFLOAT4X4 ViewProjMTX = {};
	float ShadowMapSize = 0.0f;
	float DepthBias = 0.0f;
	float NormalBias = 0.0f;
	float CascadeFarDepth = 0.0f;
};

static_assert(sizeof(ShadowConstantBufferData) == 80, "ShadowConstantBufferData must be 80 bytes");
static_assert(offsetof(ShadowConstantBufferData, ViewProjMTX) == 0, "ShadowConstantBufferData::ViewProjMTX must start at c0");
static_assert(offsetof(ShadowConstantBufferData, ShadowMapSize) == 64, "ShadowConstantBufferData::ShadowMapSize must be at c4.x");
static_assert(offsetof(ShadowConstantBufferData, DepthBias) == 68, "ShadowConstantBufferData::DepthBias must be at c4.y");
static_assert(offsetof(ShadowConstantBufferData, NormalBias) == 72, "ShadowConstantBufferData::NormalBias must be at c4.z");
static_assert(offsetof(ShadowConstantBufferData, CascadeFarDepth) == 76, "ShadowConstantBufferData::CascadeFarDepth must be at c4.w");