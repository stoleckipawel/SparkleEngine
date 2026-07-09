#pragma once

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

namespace RayTracingHitData
{
	inline constexpr std::uint32_t AbiVersion = 3u;

	inline constexpr std::uint32_t InstanceFlag_Valid = 1u << 0u;
	inline constexpr std::uint32_t InstanceFlag_Opaque = 1u << 1u;
	inline constexpr std::uint32_t InstanceFlag_TwoSided = 1u << 2u;
	inline constexpr std::uint32_t InstanceFlag_StaticMesh = 1u << 3u;

	inline constexpr std::uint32_t GeometryFlag_StaticMesh = 1u << 0u;
	inline constexpr std::uint32_t GeometryFlag_SkinnedMesh = 1u << 1u;
	inline constexpr std::uint32_t GeometryFlag_AlphaTested = 1u << 2u;
	inline constexpr std::uint32_t GeometryFlag_AlphaBlended = 1u << 3u;
	inline constexpr std::uint32_t GeometryFlag_TexturedMaterial = 1u << 4u;
	inline constexpr std::uint32_t GeometryFlag_DoubleSided = 1u << 5u;

	inline constexpr std::uint32_t Reason_None = 0u;
	inline constexpr std::uint32_t Reason_NoHit = 1u;
	inline constexpr std::uint32_t Reason_HitDataUnavailable = 2u;
	inline constexpr std::uint32_t Reason_InstanceOutOfRange = 3u;
	inline constexpr std::uint32_t Reason_InvalidInstance = 4u;
	inline constexpr std::uint32_t Reason_InvalidMaterial = 5u;
	inline constexpr std::uint32_t Reason_MissingMeshHitData = 8u;
	inline constexpr std::uint32_t Reason_InvalidPrimitive = 9u;
	inline constexpr std::uint32_t Reason_InvalidVertexIndex = 10u;
	inline constexpr std::uint32_t Reason_OneSidedBackface = 11u;
	inline constexpr std::uint32_t Reason_AlphaRejected = 13u;

	inline constexpr std::uint32_t MaterialFlag_DoubleSided = 1u << 0u;
	inline constexpr std::uint32_t MaterialFlag_Opaque = 1u << 1u;
	inline constexpr std::uint32_t MaterialFlag_AlphaTested = 1u << 2u;
	inline constexpr std::uint32_t MaterialFlag_AlphaBlended = 1u << 3u;
	inline constexpr std::uint32_t MaterialFlag_Textured = 1u << 4u;
}

struct RayTracingHitVertex
{
	DirectX::XMFLOAT3 Position = {};
	DirectX::XMFLOAT3 Normal = {};
	DirectX::XMFLOAT4 Tangent = {1.0f, 0.0f, 0.0f, 1.0f};
	DirectX::XMFLOAT2 TexCoord0 = {};
	DirectX::XMFLOAT2 Padding0 = {};
};

static_assert(std::is_standard_layout_v<RayTracingHitVertex>, "RayTracingHitVertex must be standard-layout");
static_assert(std::is_trivially_copyable_v<RayTracingHitVertex>, "RayTracingHitVertex must be trivially copyable");
static_assert(sizeof(RayTracingHitVertex) == 56, "RayTracingHitVertex must match the shader layout");

struct RayTracingHitInstance
{
	std::uint32_t FirstVertex = 0u;
	std::uint32_t FirstIndex = 0u;
	std::uint32_t VertexCount = 0u;
	std::uint32_t IndexCount = 0u;
	std::uint32_t MaterialSlot = 0u;
	std::uint32_t Flags = 0u;
	std::uint32_t GeometryFlags = 0u;
	std::uint32_t RejectionReason = RayTracingHitData::Reason_InvalidInstance;
	std::uint32_t AlphaMode = 0u;
	std::uint32_t MaterialTextureFlags = 0u;
	std::uint32_t AbiVersion = RayTracingHitData::AbiVersion;
	std::uint32_t Padding0 = 0u;
};

static_assert(std::is_standard_layout_v<RayTracingHitInstance>, "RayTracingHitInstance must be standard-layout");
static_assert(std::is_trivially_copyable_v<RayTracingHitInstance>, "RayTracingHitInstance must be trivially copyable");
static_assert(sizeof(RayTracingHitInstance) == 48, "RayTracingHitInstance must match the shader layout");

struct RayTracingHitMaterial
{
	DirectX::XMFLOAT4 BaseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	DirectX::XMFLOAT3 EmissiveColor = {};
	float Metallic = 0.0f;
	float Roughness = 0.5f;
	float F0 = 0.04f;
	float AlphaCutoff = 0.5f;
	std::uint32_t AlphaMode = 0u;
	std::uint32_t TextureFlags = 0u;
	DirectX::XMFLOAT3 SubsurfaceColor = {};
	float SubsurfaceStrength = 0.0f;
	std::uint32_t Flags = 0u;
	DirectX::XMUINT4 TextureIndices0 = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
	DirectX::XMUINT4 TextureIndices1 = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
};

static_assert(std::is_standard_layout_v<RayTracingHitMaterial>, "RayTracingHitMaterial must be standard-layout");
static_assert(std::is_trivially_copyable_v<RayTracingHitMaterial>, "RayTracingHitMaterial must be trivially copyable");
static_assert(sizeof(RayTracingHitMaterial) == 104, "RayTracingHitMaterial must match the shader layout");
