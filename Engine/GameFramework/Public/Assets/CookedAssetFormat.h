#pragma once

#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/MeshData.h"

#include <DirectXMath.h>

#include <cstdint>
#include <type_traits>

constexpr std::uint32_t MakeCookedAssetMagic(char a, char b, char c, char d) noexcept
{
	return static_cast<std::uint32_t>(static_cast<unsigned char>(a)) |
	       (static_cast<std::uint32_t>(static_cast<unsigned char>(b)) << 8u) |
	       (static_cast<std::uint32_t>(static_cast<unsigned char>(c)) << 16u) |
	       (static_cast<std::uint32_t>(static_cast<unsigned char>(d)) << 24u);
}

inline constexpr std::uint16_t kCookedAssetFormatVersionMajor = 1;
inline constexpr std::uint16_t kCookedAssetFormatVersionMinor = 0;

inline constexpr std::uint32_t kCookedSceneAssetMagic = MakeCookedAssetMagic('S', 'A', 'S', 'T');
inline constexpr std::uint32_t kCookedMeshAssetMagic = MakeCookedAssetMagic('S', 'M', 'S', 'H');
inline constexpr std::uint32_t kCookedMaterialAssetMagic = MakeCookedAssetMagic('S', 'M', 'A', 'T');
inline constexpr std::uint32_t kCookedTextureManifestMagic = MakeCookedAssetMagic('S', 'T', 'E', 'X');

inline constexpr std::int32_t kInvalidCookedTextureIndex = -1;

enum class CookedAssetReferenceKind : std::uint32_t
{
	MeshData = 0,
	MaterialData = 1,
	TextureManifest = 2,
};

enum class CookedTextureUsageFlags : std::uint32_t
{
	None = 0,
	Albedo = 1u << 0u,
	Normal = 1u << 1u,
	MetallicRoughness = 1u << 2u,
	Occlusion = 1u << 3u,
	Emissive = 1u << 4u,
};

enum class CookedTextureFormatHint : std::uint32_t
{
	Unknown = 0,
	ColorSrgb = 1,
	DataLinear = 2,
	NormalMap = 3,
};

constexpr CookedTextureUsageFlags operator|(CookedTextureUsageFlags lhs, CookedTextureUsageFlags rhs) noexcept
{
	return static_cast<CookedTextureUsageFlags>(
	    static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr CookedTextureUsageFlags& operator|=(CookedTextureUsageFlags& lhs, CookedTextureUsageFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasAnyCookedTextureUsage(CookedTextureUsageFlags value, CookedTextureUsageFlags flags) noexcept
{
	return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flags)) != 0u;
}

struct SPARKLE_ENGINE_API CookedStringRef
{
	std::uint32_t offset = 0;
	std::uint32_t length = 0;

	bool IsEmpty() const noexcept { return length == 0; }
};

struct SPARKLE_ENGINE_API CookedAssetFileHeader
{
	std::uint32_t magic = 0;
	std::uint16_t versionMajor = kCookedAssetFormatVersionMajor;
	std::uint16_t versionMinor = kCookedAssetFormatVersionMinor;
	std::uint32_t headerSize = 0;
	std::uint32_t flags = 0;
};

struct SPARKLE_ENGINE_API CookedSceneAssetHeader
{
	CookedAssetFileHeader fileHeader{};
	std::uint32_t meshCount = 0;
	std::uint32_t materialCount = 0;
	std::uint32_t textureCount = 0;
	std::uint32_t meshEntryOffset = 0;
	std::uint32_t referenceEntryOffset = 0;
	std::uint32_t referenceCount = 0;
	std::uint32_t stringTableOffset = 0;
};

struct SPARKLE_ENGINE_API CookedSceneMeshEntry
{
	std::uint64_t nameHash = 0;
	DirectX::XMFLOAT3 translation = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 rotationEuler = {0.0f, 0.0f, 0.0f};
	DirectX::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f};
	std::uint32_t materialIndex = 0;
	std::uint32_t vertexOffset = 0;
	std::uint32_t indexOffset = 0;
	std::uint32_t vertexCount = 0;
	std::uint32_t indexCount = 0;
};

struct SPARKLE_ENGINE_API CookedAssetReferenceEntry
{
	CookedAssetReferenceKind kind = CookedAssetReferenceKind::MeshData;
	CookedStringRef relativePath{};
	std::uint32_t reserved = 0;
};

struct SPARKLE_ENGINE_API CookedMeshAssetHeader
{
	CookedAssetFileHeader fileHeader{};
	std::uint32_t meshCount = 0;
	std::uint32_t meshTableOffset = 0;
	std::uint32_t vertexDataOffset = 0;
	std::uint32_t indexDataOffset = 0;
	std::uint32_t vertexStride = sizeof(VertexData);
	std::uint32_t indexStride = sizeof(std::uint32_t);
	std::uint64_t totalVertexCount = 0;
	std::uint64_t totalIndexCount = 0;
};

struct SPARKLE_ENGINE_API CookedMeshEntry
{
	std::uint64_t nameHash = 0;
	std::uint32_t vertexCount = 0;
	std::uint32_t indexCount = 0;
	std::uint32_t vertexOffset = 0;
	std::uint32_t indexOffset = 0;
};

struct SPARKLE_ENGINE_API CookedMaterialAssetHeader
{
	CookedAssetFileHeader fileHeader{};
	std::uint32_t materialCount = 0;
	std::uint32_t materialEntryOffset = 0;
	std::uint32_t stringTableOffset = 0;
	std::uint32_t reserved = 0;
};

struct SPARKLE_ENGINE_API CookedMaterialEntry
{
	CookedStringRef name{};
	DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float metallic = 0.0f;
	float roughness = 0.5f;
	float f0 = 0.04f;
	DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
	std::uint32_t alphaMode = static_cast<std::uint32_t>(AlphaMode::Opaque);
	float alphaCutoff = 0.5f;
	std::int32_t albedoTextureIndex = kInvalidCookedTextureIndex;
	std::int32_t normalTextureIndex = kInvalidCookedTextureIndex;
	std::int32_t metallicRoughnessTextureIndex = kInvalidCookedTextureIndex;
	std::int32_t occlusionTextureIndex = kInvalidCookedTextureIndex;
	std::int32_t emissiveTextureIndex = kInvalidCookedTextureIndex;
	std::uint32_t reserved = 0;
};

struct SPARKLE_ENGINE_API CookedTextureManifestHeader
{
	CookedAssetFileHeader fileHeader{};
	std::uint32_t textureCount = 0;
	std::uint32_t textureEntryOffset = 0;
	std::uint32_t stringTableOffset = 0;
	std::uint32_t reserved = 0;
};

struct SPARKLE_ENGINE_API CookedTextureEntry
{
	CookedStringRef cookedTexturePath{};
	CookedStringRef sourceName{};
	std::uint64_t sourcePathHash = 0;
	CookedTextureUsageFlags usageFlags = CookedTextureUsageFlags::None;
	CookedTextureFormatHint formatHint = CookedTextureFormatHint::Unknown;
	std::uint32_t reserved = 0;
};

static_assert(std::is_trivially_copyable_v<CookedStringRef>);
static_assert(std::is_trivially_copyable_v<CookedAssetFileHeader>);
static_assert(std::is_trivially_copyable_v<CookedSceneAssetHeader>);
static_assert(std::is_trivially_copyable_v<CookedSceneMeshEntry>);
static_assert(std::is_trivially_copyable_v<CookedAssetReferenceEntry>);
static_assert(std::is_trivially_copyable_v<CookedMeshAssetHeader>);
static_assert(std::is_trivially_copyable_v<CookedMeshEntry>);
static_assert(std::is_trivially_copyable_v<CookedMaterialAssetHeader>);
static_assert(std::is_trivially_copyable_v<CookedMaterialEntry>);
static_assert(std::is_trivially_copyable_v<CookedTextureManifestHeader>);
static_assert(std::is_trivially_copyable_v<CookedTextureEntry>);
