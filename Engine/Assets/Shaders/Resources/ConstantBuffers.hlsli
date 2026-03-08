#pragma once

// =============================================================================
// Constant Buffer Definitions
// =============================================================================
// Layout: b0 = PerFrame, b1 = PerView, b2 = PerObject VS, b3 = PerObject PS

// -----------------------------------------------------------------------------
// Per-Frame CB (b0) — updated once per CPU frame, shared by all draws
// -----------------------------------------------------------------------------
cbuffer PerFrameConstantBufferData : register(b0)
{
	uint FrameIndex;        // Current frame number
	float TotalTime;        // Seconds since engine start (unscaled)
	float DeltaTime;        // Seconds since last frame (unscaled)
	float ScaledTotalTime;  // Seconds of scaled/game time since start (stops when paused)
	float ScaledDeltaTime;  // Seconds since last frame (scaled, 0 when paused)
	uint ViewModeIndex;     // Current renderer debug view mode (see Debug/ViewModes.hlsli)

	float2 ViewportSize;     // Render target width, height
	float2 ViewportSizeInv;  // 1.0 / width, 1.0 / height

	// rest of 256-byte slot is intentionally unused/pad
};

// -----------------------------------------------------------------------------
// Per-View CB (b1) — updated per camera/view (main, shadow, reflection, etc.)
// -----------------------------------------------------------------------------
cbuffer PerViewConstantBufferData : register(b1)
{
	row_major float4x4 ViewMTX;        // World -> View
	row_major float4x4 ProjectionMTX;  // View -> Clip
	row_major float4x4 ViewProjMTX;    // World -> Clip (precomputed to save GPU work)

	float3 CameraPosition;  // World-space camera position
	float NearZ;            // Near clip plane

	float FarZ;              // Far clip plane
	float3 CameraDirection;  // World-space camera forward

	float3 SunDirection;  // World-space sun light direction
	float SunIntensity;   // Sun light intensity multiplier

	float3 SunColor;     // Sun light color (linear RGB)
	float _padPerView0;  // Pad to 256-byte boundary
};

// -----------------------------------------------------------------------------
// Per-Object VS CB (b2) — updated per draw call (transforms)
// -----------------------------------------------------------------------------
cbuffer PerObjectVSConstantBufferData : register(b2)
{
	row_major float4x4 WorldMTX;              // Local -> World
	row_major float3x3 WorldInvTransposeMTX;  // Normal transform (3x3) -> correct under non-uniform scale

	// remaining space in the 256-byte slot is reserved for future use
};

// -----------------------------------------------------------------------------
// Per-Object PS CB (b3) — updated per draw call (material scalars)
// -----------------------------------------------------------------------------
cbuffer PerObjectPSConstantBufferData : register(b3)
{
	float4 BaseColor;  // RGBA base/albedo color or tint

	float3 EmissiveColor;  // Emissive multiplier/fallback color
	float Metallic;        // PBR metallic [0,1]

	float Roughness;    // PBR roughness [0,1]
	float F0;           // PBR reflectance at normal incidence
	float AlphaCutoff;  // Alpha test threshold for masked materials
	uint AlphaMode;     // Matches MaterialDesc::AlphaMode numeric values

	uint TextureFlags;     // Bitmask of authored textures present on the material
	float3 _padPerObjectPS0;

	// remaining space reserved
};
