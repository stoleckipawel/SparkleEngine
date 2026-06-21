#pragma once

#include "RHI/Public/Resources/MeshInstanceShaderData.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

struct RTIndirectSpecularHitVertex
{
	DirectX::XMFLOAT3 Position = {};
	DirectX::XMFLOAT3 Normal = {};
	DirectX::XMFLOAT4 Tangent = {1.0f, 0.0f, 0.0f, 1.0f};
	DirectX::XMFLOAT2 TexCoord0 = {};
	DirectX::XMFLOAT2 Padding0 = {};
};

static_assert(std::is_standard_layout_v<RTIndirectSpecularHitVertex>, "RTIndirectSpecularHitVertex must be standard-layout");
static_assert(std::is_trivially_copyable_v<RTIndirectSpecularHitVertex>, "RTIndirectSpecularHitVertex must be trivially copyable");
static_assert(sizeof(RTIndirectSpecularHitVertex) == 56, "RTIndirectSpecularHitVertex must match the shader layout");

inline constexpr std::uint32_t RTIndirectSpecularHitDataAbiVersion = 2u;

inline constexpr std::uint32_t RTIndirectSpecularHitInstanceFlag_Valid = 1u << 0u;
inline constexpr std::uint32_t RTIndirectSpecularHitInstanceFlag_Opaque = 1u << 1u;
inline constexpr std::uint32_t RTIndirectSpecularHitInstanceFlag_TwoSided = 1u << 2u;
inline constexpr std::uint32_t RTIndirectSpecularHitInstanceFlag_StaticMesh = 1u << 3u;

inline constexpr std::uint32_t RTIndirectSpecularHitGeometryFlag_StaticMesh = 1u << 0u;
inline constexpr std::uint32_t RTIndirectSpecularHitGeometryFlag_SkinnedMesh = 1u << 1u;
inline constexpr std::uint32_t RTIndirectSpecularHitGeometryFlag_AlphaTested = 1u << 2u;
inline constexpr std::uint32_t RTIndirectSpecularHitGeometryFlag_AlphaBlended = 1u << 3u;
inline constexpr std::uint32_t RTIndirectSpecularHitGeometryFlag_TexturedMaterial = 1u << 4u;
inline constexpr std::uint32_t RTIndirectSpecularHitGeometryFlag_DoubleSided = 1u << 5u;

inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_None = 0u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_NoHit = 1u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_HitDataUnavailable = 2u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_InstanceOutOfRange = 3u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_InvalidInstance = 4u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_InvalidMaterial = 5u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_UnsupportedSkinned = 6u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_UnsupportedAlphaMode = 7u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_MissingMeshHitData = 8u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_InvalidPrimitive = 9u;
inline constexpr std::uint32_t RTIndirectSpecularHitFallbackReason_InvalidVertexIndex = 10u;

inline constexpr std::uint32_t RTIndirectSpecularHitMaterialFlag_DoubleSided = 1u << 0u;
inline constexpr std::uint32_t RTIndirectSpecularHitMaterialFlag_Opaque = 1u << 1u;
inline constexpr std::uint32_t RTIndirectSpecularHitMaterialFlag_AlphaTested = 1u << 2u;
inline constexpr std::uint32_t RTIndirectSpecularHitMaterialFlag_AlphaBlended = 1u << 3u;
inline constexpr std::uint32_t RTIndirectSpecularHitMaterialFlag_Textured = 1u << 4u;

struct RTIndirectSpecularHitInstance
{
	std::uint32_t FirstVertex = 0u;
	std::uint32_t FirstIndex = 0u;
	std::uint32_t VertexCount = 0u;
	std::uint32_t IndexCount = 0u;
	std::uint32_t MaterialSlot = 0u;
	std::uint32_t Flags = 0u;
	std::uint32_t GeometryFlags = 0u;
	std::uint32_t FallbackReason = RTIndirectSpecularHitFallbackReason_InvalidInstance;
	std::uint32_t AlphaMode = 0u;
	std::uint32_t MaterialTextureFlags = 0u;
	std::uint32_t AbiVersion = RTIndirectSpecularHitDataAbiVersion;
	std::uint32_t Padding0 = 0u;
};

static_assert(std::is_standard_layout_v<RTIndirectSpecularHitInstance>, "RTIndirectSpecularHitInstance must be standard-layout");
static_assert(std::is_trivially_copyable_v<RTIndirectSpecularHitInstance>, "RTIndirectSpecularHitInstance must be trivially copyable");
static_assert(sizeof(RTIndirectSpecularHitInstance) == 48, "RTIndirectSpecularHitInstance must match the shader layout");

struct RTIndirectSpecularHitMaterial
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
	DirectX::XMFLOAT2 Padding0 = {};
};

static_assert(std::is_standard_layout_v<RTIndirectSpecularHitMaterial>, "RTIndirectSpecularHitMaterial must be standard-layout");
static_assert(std::is_trivially_copyable_v<RTIndirectSpecularHitMaterial>, "RTIndirectSpecularHitMaterial must be trivially copyable");
static_assert(sizeof(RTIndirectSpecularHitMaterial) == 80, "RTIndirectSpecularHitMaterial must match the shader layout");
