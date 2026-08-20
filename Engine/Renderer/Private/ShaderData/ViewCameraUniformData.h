#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <type_traits>

struct alignas(256) ViewCameraUniformData
{
	DirectX::XMFLOAT4X4 ViewMTX = {};
	DirectX::XMFLOAT4X4 ProjectionMTX = {};
	DirectX::XMFLOAT4X4 ViewProjMTX = {};
	DirectX::XMFLOAT4X4 InvViewMTX = {};
	DirectX::XMFLOAT4X4 InvProjectionMTX = {};
	DirectX::XMFLOAT3 Position = {};
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	DirectX::XMFLOAT3 Direction = {0.0f, 0.0f, 1.0f};
};
static_assert(std::is_standard_layout_v<ViewCameraUniformData>);
static_assert(std::is_trivially_copyable_v<ViewCameraUniformData>);
static_assert(alignof(ViewCameraUniformData) >= 256);
static_assert(sizeof(ViewCameraUniformData) % 256 == 0);
static_assert(sizeof(ViewCameraUniformData) <= 64 * 1024);
static_assert(sizeof(ViewCameraUniformData) == 512u, "View camera uniform data must fit two aligned CBV slots");
static_assert(offsetof(ViewCameraUniformData, ViewMTX) == 0u);
static_assert(offsetof(ViewCameraUniformData, ProjectionMTX) == 64u);
static_assert(offsetof(ViewCameraUniformData, ViewProjMTX) == 128u);
static_assert(offsetof(ViewCameraUniformData, InvViewMTX) == 192u);
static_assert(offsetof(ViewCameraUniformData, InvProjectionMTX) == 256u);
static_assert(offsetof(ViewCameraUniformData, Position) == 320u);
static_assert(offsetof(ViewCameraUniformData, NearZ) == 332u);
static_assert(offsetof(ViewCameraUniformData, FarZ) == 336u);
static_assert(offsetof(ViewCameraUniformData, Direction) == 340u);
