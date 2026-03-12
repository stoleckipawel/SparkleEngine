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

struct SPARKLE_ENGINE_API MaterialDesc
{
	std::string name;

	DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float metallic = 0.0f;
	float roughness = 0.5f;
	float f0 = 0.04f;
	DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
	AlphaMode alphaMode = AlphaMode::Opaque;
	float alphaCutoff = 0.5f;

	std::optional<std::filesystem::path> albedoTexture;
	std::optional<std::filesystem::path> normalTexture;
	std::optional<std::filesystem::path> metallicRoughnessTexture;
	std::optional<std::filesystem::path> occlusionTexture;
	std::optional<std::filesystem::path> emissiveTexture;
};
