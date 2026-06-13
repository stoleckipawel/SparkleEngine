#pragma once

#include "ImportedTextureSource.h"

#include <DirectXMath.h>

#include <cstdint>
#include <string>
#include <vector>

enum class ImportedAlphaMode : std::uint32_t
{
	Opaque = 0,
	Mask = 1,
	Blend = 2,
};

struct ImportedMaterial
{
	std::string name;

	DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float metallic = 0.0f;
	float roughness = 0.5f;
	float f0 = 0.04f;
	DirectX::XMFLOAT3 subsurfaceColor = {0.0f, 0.0f, 0.0f};
	float subsurfaceStrength = 0.0f;
	DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
	ImportedAlphaMode alphaMode = ImportedAlphaMode::Opaque;
	float alphaCutoff = 0.5f;
	bool doubleSided = false;

	std::vector<ImportedTextureSource> textureSources;
};
