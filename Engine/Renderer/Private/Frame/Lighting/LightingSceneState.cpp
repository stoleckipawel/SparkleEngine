#include "../../PCH.h"
#include "Frame/Lighting/LightingSceneState.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Lighting/LightingStateHash.h"
#include "RHI/Public/Resources/Texture.h"

#include <cstdint>
#include <vector>

namespace
{
	template <typename TValue> std::uint64_t AppendCount(std::uint64_t hash, const std::vector<TValue>& values) noexcept
	{
		return Hash::ContinueFnv1a64Value(hash, static_cast<std::uint64_t>(values.size()));
	}

	std::uint64_t AppendLightState(std::uint64_t hash, const DirectionalLight& light) noexcept
	{
		hash = LightingStateHash::AppendFloat3(hash, light.direction);
		hash = Hash::ContinueFnv1a64Value(hash, light.intensity);
		hash = LightingStateHash::AppendFloat3(hash, light.color);
		hash = Hash::ContinueFnv1a64Value(hash, light.angularDiameterRadians);
		return LightingStateHash::AppendBool(hash, light.castShadow);
	}

	std::uint64_t AppendLightState(std::uint64_t hash, const PointLight& light) noexcept
	{
		hash = LightingStateHash::AppendFloat3(hash, light.position);
		hash = Hash::ContinueFnv1a64Value(hash, light.range);
		hash = LightingStateHash::AppendFloat3(hash, light.color);
		hash = Hash::ContinueFnv1a64Value(hash, light.intensity);
		hash = Hash::ContinueFnv1a64Value(hash, light.sourceRadius);
		return LightingStateHash::AppendBool(hash, light.castShadow);
	}

	std::uint64_t AppendLightState(std::uint64_t hash, const SpotLight& light) noexcept
	{
		hash = LightingStateHash::AppendFloat3(hash, light.position);
		hash = Hash::ContinueFnv1a64Value(hash, light.range);
		hash = Hash::ContinueFnv1a64Value(hash, light.sourceRadius);
		hash = LightingStateHash::AppendFloat3(hash, light.direction);
		hash = Hash::ContinueFnv1a64Value(hash, light.innerConeCosine);
		hash = LightingStateHash::AppendFloat3(hash, light.color);
		hash = Hash::ContinueFnv1a64Value(hash, light.intensity);
		hash = Hash::ContinueFnv1a64Value(hash, light.outerConeCosine);
		return LightingStateHash::AppendBool(hash, light.castShadow);
	}

	std::uint64_t AppendLightState(std::uint64_t hash, const RectLight& light) noexcept
	{
		hash = LightingStateHash::AppendFloat3(hash, light.position);
		hash = Hash::ContinueFnv1a64Value(hash, light.width);
		hash = LightingStateHash::AppendFloat3(hash, light.direction);
		hash = Hash::ContinueFnv1a64Value(hash, light.height);
		hash = LightingStateHash::AppendFloat3(hash, light.tangent);
		hash = Hash::ContinueFnv1a64Value(hash, light.luminance);
		hash = LightingStateHash::AppendFloat3(hash, light.color);
		return LightingStateHash::AppendBool(hash, light.castShadow);
	}

	template <typename TLight> std::uint64_t AppendLightsState(std::uint64_t hash, const std::vector<TLight>& lights) noexcept
	{
		hash = AppendCount(hash, lights);
		for (const TLight& light : lights)
		{
			hash = AppendLightState(hash, light);
		}
		return hash;
	}

	std::uint64_t AppendMeshState(std::uint64_t hash, const MeshDraw& draw) noexcept
	{
		hash = LightingStateHash::AppendMatrix(hash, draw.Transform.WorldMatrix);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Material.Slot);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Skinning.SkeletonAssetId);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Skinning.JointMatrixOffset);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Geometry.MeshKind);
		return Hash::ContinueFnv1a64Value(hash, reinterpret_cast<std::uintptr_t>(draw.Geometry.GpuMesh));
	}

	std::uint64_t AppendMaterialState(std::uint64_t hash, const MaterialData& material) noexcept
	{
		hash = LightingStateHash::AppendFloat4(hash, material.baseColor);
		hash = Hash::ContinueFnv1a64Value(hash, material.metallic);
		hash = Hash::ContinueFnv1a64Value(hash, material.roughness);
		hash = Hash::ContinueFnv1a64Value(hash, material.f0);
		hash = LightingStateHash::AppendFloat3(hash, material.subsurfaceColor);
		hash = Hash::ContinueFnv1a64Value(hash, material.subsurfaceStrength);
		hash = LightingStateHash::AppendFloat3(hash, material.emissiveColor);
		hash = Hash::ContinueFnv1a64Value(hash, material.alphaMode);
		hash = Hash::ContinueFnv1a64Value(hash, material.alphaCutoff);
		hash = Hash::ContinueFnv1a64Value(hash, material.textureFlags);
		hash = LightingStateHash::AppendBool(hash, material.doubleSided);
		for (const std::uint32_t textureIndex : material.materialTextureIndices)
		{
			hash = Hash::ContinueFnv1a64Value(hash, textureIndex);
		}
		return Hash::ContinueFnv1a64Value(hash, reinterpret_cast<std::uintptr_t>(material.textureBindingSet));
	}

	std::uint64_t AppendSkyState(std::uint64_t hash, const RenderSkyData& sky) noexcept
	{
		const Texture* skyTexture = sky.skyTexture;
		hash = LightingStateHash::AppendBool(hash, sky.enabled);
		hash = LightingStateHash::AppendFloat3(hash, sky.color);
		hash = Hash::ContinueFnv1a64Value(hash, sky.intensity);
		hash = LightingStateHash::AppendBool(hash, skyTexture != nullptr);
		if (skyTexture == nullptr)
		{
			return hash;
		}

		hash = Hash::ContinueFnv1a64Value(hash, reinterpret_cast<std::uintptr_t>(skyTexture));
		const TextureRuntimeInfo runtimeInfo = skyTexture->GetRuntimeInfo();
		hash = LightingStateHash::AppendBool(hash, runtimeInfo.IsValid);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.Width);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.Height);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.ArraySize);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.Dimension);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.Format);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.FormatIntent);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.MipCount);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.EstimatedByteSize);
		hash = Hash::ContinueFnv1a64Value(hash, runtimeInfo.GpuShaderResourceViewId);
		hash = Hash::ContinueFnv1a64Value(hash, static_cast<std::uint64_t>(runtimeInfo.FormatName.size()));
		return Hash::ContinueFnv1a64(hash, runtimeInfo.FormatName.data(), runtimeInfo.FormatName.size());
	}
}

std::uint64_t BuildLightingSceneStateKey(const FrameContext& frame) noexcept
{
	std::uint64_t hash = Hash::kFnv64OffsetBasis;
	hash = AppendSkyState(hash, frame.sceneData.sky);
	hash = AppendLightsState(hash, frame.sceneData.directionalLights);
	hash = AppendLightsState(hash, frame.sceneData.pointLights);
	hash = AppendLightsState(hash, frame.sceneData.spotLights);
	hash = AppendLightsState(hash, frame.sceneData.rectLights);

	hash = AppendCount(hash, frame.sceneData.meshInstances);
	for (const MeshDraw& draw : frame.sceneData.meshInstances)
	{
		hash = AppendMeshState(hash, draw);
	}

	hash = AppendCount(hash, frame.sceneData.jointMatrices);
	for (const DirectX::XMFLOAT4X4& jointMatrix : frame.sceneData.jointMatrices)
	{
		hash = LightingStateHash::AppendMatrix(hash, jointMatrix);
	}

	hash = AppendCount(hash, frame.sceneData.materials);
	for (const MaterialData& material : frame.sceneData.materials)
	{
		hash = AppendMaterialState(hash, material);
	}

	hash = Hash::ContinueFnv1a64Value(hash, reinterpret_cast<std::uintptr_t>(frame.sceneData.materialTextureTable));
	hash = Hash::ContinueFnv1a64Value(hash, frame.sceneData.materialTextureTableDescriptorCount);
	hash = LightingStateHash::AppendBool(hash, frame.sceneData.materialTextureTableValid);
	return Hash::FinalizeFnv1a64(hash);
}
