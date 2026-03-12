// ============================================================================
// D3D12ConstantBufferData.h
// ----------------------------------------------------------------------------
// POD structures for GPU constant buffers, mirrored in HLSL (Common.hlsli).
//
#pragma once
#include <cstddef>
#include <DirectXMath.h>
#include <cstdint>
#include <type_traits>

// ============================================================================
// CBV Validation Macro
// ============================================================================

// Helper macro to validate CBV layout/size for all constant-buffer structs.
// Example: CBV_CHECK(MyCbStruct);
// Validate that CB types are safe to memcpy to GPU memory and match HLSL layout
#define CBV_CHECK(Type)                                                                      \
	static_assert(std::is_standard_layout_v<Type>, #Type " must be standard-layout");        \
	static_assert(std::is_trivially_copyable_v<Type>, #Type " must be trivially-copyable");  \
	static_assert(alignof(Type) >= 256, #Type " must be 256-byte aligned");                  \
	static_assert(sizeof(Type) % 256 == 0, #Type " must occupy whole 256-byte CBV slot(s)"); \
	static_assert(sizeof(Type) <= 64 * 1024, #Type " must be <= 64KB")

//------------------------------------------------------------------------------
// Per-Frame CB (b0) - updated once per CPU frame, shared by all draws
//------------------------------------------------------------------------------
struct alignas(256) PerFrameConstantBufferData
{
	uint32_t FrameIndex;     // Current frame number
	float TotalTime;         // Seconds since engine start (unscaled)
	float DeltaTime;         // Seconds since last frame (unscaled)
	float ScaledTotalTime;   // Seconds of scaled/game time since start (stops when paused)
	float ScaledDeltaTime;   // Seconds since last frame (scaled, 0 when paused)
	uint32_t ViewModeIndex;  // Current renderer debug view mode

	DirectX::XMFLOAT2 ViewportSize;     // Render target width, height
	DirectX::XMFLOAT2 ViewportSizeInv;  // 1.0 / width, 1.0 / height

	// rest of 256-byte slot is intentionally unused/pad
};
CBV_CHECK(PerFrameConstantBufferData);

//------------------------------------------------------------------------------
// Per-View CB (b1) - updated per camera/view (main, shadow, reflection, etc.)
//------------------------------------------------------------------------------
struct alignas(256) PerViewConstantBufferData
{
	DirectX::XMFLOAT4X4 ViewMTX;        // World -> View
	DirectX::XMFLOAT4X4 ProjectionMTX;  // View -> Clip
	DirectX::XMFLOAT4X4 ViewProjMTX;    // World -> Clip (precomputed to save GPU work)

	DirectX::XMFLOAT3 CameraPosition;  // World-space camera position
	float NearZ;                       // Near clip plane

	float FarZ;                         // Far clip plane
	DirectX::XMFLOAT3 CameraDirection;  // World-space camera forward

	DirectX::XMFLOAT3 SunDirection;  // World-space sun light direction
	float SunIntensity;              // Sun light intensity multiplier

	DirectX::XMFLOAT3 SunColor;  // Sun light color (linear RGB)
	float _padPerView0;          // Pad to 256-byte boundary
};
CBV_CHECK(PerViewConstantBufferData);

//------------------------------------------------------------------------------
// Per-Object VS CB (b2) - updated per draw call (transforms)
//------------------------------------------------------------------------------
struct alignas(256) PerObjectVSConstantBufferData
{
	DirectX::XMFLOAT4X4 WorldMTX;              // Local -> World
	DirectX::XMFLOAT3X4 WorldInvTransposeMTX;  // Normal matrix (inverse-transpose, 3x4 = 48 bytes matches HLSL float3x3 cbuffer packing)

	// remaining space in the 256-byte slot is reserved for future use
};
CBV_CHECK(PerObjectVSConstantBufferData);
//------------------------------------------------------------------------------
// Per-Object PS CB (b3) - updated per draw call (material scalars)
//------------------------------------------------------------------------------
struct alignas(256) PerObjectPSConstantBufferData
{
	DirectX::XMFLOAT4 BaseColor;  // RGBA base/albedo color or tint

	DirectX::XMFLOAT3 EmissiveColor;  // Emissive multiplier/fallback color
	float Metallic;                  // PBR metallic [0,1]

	float Roughness;     // PBR roughness [0,1]
	float F0;            // PBR reflectance at normal incidence
	float AlphaCutoff;   // Alpha test threshold for masked materials
	uint32_t AlphaMode;  // Matches MaterialDesc::AlphaMode numeric values

	uint32_t TextureFlags;  // Bitmask of authored textures present on the material
	DirectX::XMFLOAT3 _padPerObjectPS0 = {0.0f, 0.0f, 0.0f};

	// remaining space reserved
};
CBV_CHECK(PerObjectPSConstantBufferData);
static_assert(offsetof(PerObjectPSConstantBufferData, BaseColor) == 0, "PerObjectPSConstantBufferData::BaseColor must start at c0");
static_assert(offsetof(PerObjectPSConstantBufferData, EmissiveColor) == 16, "PerObjectPSConstantBufferData::EmissiveColor must start at c1");
static_assert(offsetof(PerObjectPSConstantBufferData, Metallic) == 28, "PerObjectPSConstantBufferData::Metallic must share c1.w");
static_assert(offsetof(PerObjectPSConstantBufferData, Roughness) == 32, "PerObjectPSConstantBufferData::Roughness must start at c2.x");
static_assert(offsetof(PerObjectPSConstantBufferData, F0) == 36, "PerObjectPSConstantBufferData::F0 must start at c2.y");
static_assert(offsetof(PerObjectPSConstantBufferData, AlphaCutoff) == 40, "PerObjectPSConstantBufferData::AlphaCutoff must start at c2.z");
static_assert(offsetof(PerObjectPSConstantBufferData, AlphaMode) == 44, "PerObjectPSConstantBufferData::AlphaMode must start at c2.w");
static_assert(offsetof(PerObjectPSConstantBufferData, TextureFlags) == 48, "PerObjectPSConstantBufferData::TextureFlags must start at c3.x");

//------------------------------------------------------------------------------
// Per-Instance Data (structured buffer element) - updated per instance
// TODO: Implement when instanced rendering is needed
//------------------------------------------------------------------------------
