#pragma once

#include "RHI/Public/Resources/MeshInstanceShaderData.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

struct RTIndirectSpecularHitVertex
{
	DirectX::XMFLOAT3 Position = {};
	DirectX::XMFLOAT3 Normal = {};
};

static_assert(std::is_standard_layout_v<RTIndirectSpecularHitVertex>, "RTIndirectSpecularHitVertex must be standard-layout");
static_assert(std::is_trivially_copyable_v<RTIndirectSpecularHitVertex>, "RTIndirectSpecularHitVertex must be trivially copyable");
static_assert(sizeof(RTIndirectSpecularHitVertex) == 24, "RTIndirectSpecularHitVertex must match the shader layout");

struct RTIndirectSpecularHitInstance
{
	std::uint32_t FirstVertex = 0u;
	std::uint32_t FirstIndex = 0u;
	std::uint32_t VertexCount = 0u;
	std::uint32_t IndexCount = 0u;
	std::uint32_t MaterialSlot = 0u;
	std::uint32_t Flags = 0u;
	std::uint32_t Padding0 = 0u;
	std::uint32_t Padding1 = 0u;
};

inline constexpr std::uint32_t RTIndirectSpecularHitInstanceFlag_Valid = 1u << 0u;

static_assert(std::is_standard_layout_v<RTIndirectSpecularHitInstance>, "RTIndirectSpecularHitInstance must be standard-layout");
static_assert(std::is_trivially_copyable_v<RTIndirectSpecularHitInstance>, "RTIndirectSpecularHitInstance must be trivially copyable");
static_assert(sizeof(RTIndirectSpecularHitInstance) == 32, "RTIndirectSpecularHitInstance must match the shader layout");

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
	DirectX::XMFLOAT3 Padding0 = {};
};

static_assert(std::is_standard_layout_v<RTIndirectSpecularHitMaterial>, "RTIndirectSpecularHitMaterial must be standard-layout");
static_assert(std::is_trivially_copyable_v<RTIndirectSpecularHitMaterial>, "RTIndirectSpecularHitMaterial must be trivially copyable");
static_assert(sizeof(RTIndirectSpecularHitMaterial) == 80, "RTIndirectSpecularHitMaterial must match the shader layout");
