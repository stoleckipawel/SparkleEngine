#pragma once

#include "Core/Public/Assets/TextureGroup.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "ShaderData/PerObjectConstantBufferData.h"

#include <DirectXMath.h>
#include <array>
#include <cstdint>

struct MaterialDesc;
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

inline constexpr std::uint32_t InvalidMaterialTextureIndex = UINT32_MAX;

struct MaterialGpuHandle final
{
	std::uint32_t Index = UINT32_MAX;
	std::uint64_t Generation = 0u;

	explicit operator bool() const noexcept;
	bool operator==(const MaterialGpuHandle&) const noexcept;
};

struct MaterialData
{
	MaterialGpuHandle gpuHandle = {};
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
	std::array<std::uint32_t, MaterialTextureSlots::Count> materialTextureIndices = {
	    InvalidMaterialTextureIndex,
	    InvalidMaterialTextureIndex,
	    InvalidMaterialTextureIndex,
	    InvalidMaterialTextureIndex,
	    InvalidMaterialTextureIndex,
	    InvalidMaterialTextureIndex,
	    InvalidMaterialTextureIndex,
	    InvalidMaterialTextureIndex};

	RhiDescriptorTableBinding rasterTextureTable = {};

	static MaterialData FromDesc(const MaterialDesc& desc);
	PerObjectPSConstantBufferData ToPerObjectPSData() const;
};
