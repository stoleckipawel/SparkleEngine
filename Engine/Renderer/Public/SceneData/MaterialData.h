// =============================================================================
// MaterialData.h — PBR Material Parameters
// =============================================================================

#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "D3D12/Resources/D3D12ConstantBufferData.h"

#include <DirectXMath.h>
#include <cstdint>
#include <d3d12.h>

struct MaterialDesc;

namespace MaterialTextureFlags
{
	constexpr std::uint32_t Albedo = 0x01;
	constexpr std::uint32_t Normal = 0x02;
	constexpr std::uint32_t MetallicRoughness = 0x04;
	constexpr std::uint32_t Occlusion = 0x08;
	constexpr std::uint32_t Emissive = 0x10;
}

// =============================================================================
// MaterialData
// =============================================================================

/// Renderer-facing material package. Holds scalar fallbacks and the persistent
/// bindful texture-table handle used at draw time.
struct SPARKLE_RENDERER_API MaterialData
{
	DirectX::XMFLOAT4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float metallic = 0.0f;
	float roughness = 0.5f;
	float f0 = 0.04f;  // Fresnel reflectance at normal incidence
	DirectX::XMFLOAT3 emissiveColor = {0.0f, 0.0f, 0.0f};
	std::uint32_t alphaMode = 0;  // Matches AlphaMode::Opaque numeric values
	float alphaCutoff = 0.5f;
	std::uint32_t textureFlags = 0;
	// Base GPU handle for the material's contiguous 5-SRV table (t0-t4).
	D3D12_GPU_DESCRIPTOR_HANDLE textureTableGpuHandle = {};

	/// Creates a MaterialData from a CPU-side MaterialDesc.
	[[nodiscard]] static MaterialData FromDesc(const MaterialDesc& desc);

	/// Builds the GPU constant buffer data from this material.
	[[nodiscard]] PerObjectPSConstantBufferData ToPerObjectPSData() const
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
		return data;
	}
};
