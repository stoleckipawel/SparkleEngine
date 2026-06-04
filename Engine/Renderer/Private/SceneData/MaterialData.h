#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "Renderer/Public/RendererAPI.h"
#include "RHI/Public/Resources/RenderConstantBufferData.h"

#include <DirectXMath.h>
#include <cstdint>

struct MaterialDesc;
class RenderBindingSet;

namespace MaterialTextureSlots
{
	constexpr std::uint32_t BaseColor = 0;
	constexpr std::uint32_t Normal = 1;
	constexpr std::uint32_t Roughness = 2;
	constexpr std::uint32_t Metallic = 3;
	constexpr std::uint32_t Occlusion = 4;
	constexpr std::uint32_t Emissive = 5;
	constexpr std::uint32_t SubsurfaceColor = 6;
	constexpr std::uint32_t SubsurfaceStrength = 7;
	constexpr std::uint32_t Count = 8;
}

struct SPARKLE_RENDERER_API MaterialData
{
	DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float metallic = 0.0f;
	float roughness = 0.5f;
	float f0 = 0.04f;
	DirectX::XMFLOAT3 subsurfaceColor = {0.0f, 0.0f, 0.0f};
	float subsurfaceStrength = 0.0f;
	DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
	std::uint32_t alphaMode = 0;
	float alphaCutoff = 0.5f;
	std::uint32_t textureFlags = 0;
	bool doubleSided = false;

	const RenderBindingSet* textureBindingSet = nullptr;

	static MaterialData FromDesc(const MaterialDesc& desc);

	PerObjectPSConstantBufferData ToPerObjectPSData() const
	{
		PerObjectPSConstantBufferData data{};
		data.BaseColor = baseColor;
		data.EmissiveColor = emissiveColor;
		data.Metallic = metallic;
		data.Roughness = roughness;
		data.F0 = f0;
		data.AlphaCutoff = alphaCutoff;
		data.AlphaMode = alphaMode;
		data.TextureFlags = textureFlags;
		data.SubsurfaceColor = subsurfaceColor;
		data.SubsurfaceStrength = subsurfaceStrength;
		return data;
	}
};
