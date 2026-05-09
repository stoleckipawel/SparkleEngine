#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <DirectXMath.h>
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
	DirectX::XMFLOAT3 subsurfaceColor = {0.0f, 0.0f, 0.0f};
	float subsurfaceStrength = 0.0f;
	DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
	AlphaMode alphaMode = AlphaMode::Opaque;
	float alphaCutoff = 0.5f;

	std::optional<std::filesystem::path> albedoTexture;
	std::optional<std::filesystem::path> normalTexture;
	std::optional<std::filesystem::path> roughnessTexture;
	std::optional<std::filesystem::path> metallicTexture;
	std::optional<std::filesystem::path> occlusionTexture;
	std::optional<std::filesystem::path> emissiveTexture;
	std::optional<std::filesystem::path> subsurfaceColorTexture;
	std::optional<std::filesystem::path> subsurfaceStrengthTexture;

	void SetTexturePath(TextureGroup textureGroup, const std::optional<std::filesystem::path>& texturePath);
};
