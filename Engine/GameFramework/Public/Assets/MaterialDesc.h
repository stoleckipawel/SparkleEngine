// =============================================================================
// MaterialDesc.h - CPU-Side PBR Material Description
// =============================================================================
//
// Describes a PBR material loaded from glTF or other asset sources.
// Pure data - no GPU handles, no rendering dependencies.
//
#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

enum class AlphaMode : std::uint32_t
{
	Opaque = 0,
	Mask = 1,
	Blend = 2,
};

// =============================================================================
// MaterialDesc
// =============================================================================

struct SPARKLE_ENGINE_API MaterialDesc
{
	// -------------------------------------------------------------------------
	// Identity
	// -------------------------------------------------------------------------

	std::string name;

	// -------------------------------------------------------------------------
	// PBR Parameters
	// -------------------------------------------------------------------------

	DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float metallic = 0.0f;
	float roughness = 0.5f;
	float f0 = 0.04f;  // Fresnel reflectance at normal incidence (dielectric default)
	DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
	AlphaMode alphaMode = AlphaMode::Opaque;
	float alphaCutoff = 0.5f;

	// -------------------------------------------------------------------------
	// Texture Paths (relative to asset root)
	// -------------------------------------------------------------------------

	std::optional<std::filesystem::path> albedoTexture;
	std::optional<std::filesystem::path> normalTexture;
	std::optional<std::filesystem::path> metallicRoughnessTexture;
	std::optional<std::filesystem::path> occlusionTexture;
	std::optional<std::filesystem::path> emissiveTexture;
};
