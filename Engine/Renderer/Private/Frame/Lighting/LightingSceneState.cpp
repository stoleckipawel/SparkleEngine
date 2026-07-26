#include "../../PCH.h"
#include "Frame/Lighting/LightingSceneState.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Lighting/LightingStateHash.h"
#include "Lighting/LightingCVars.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "Textures/RendererTexture.h"

#include <cstdint>
#include <vector>

class LightingSceneStateOperations final
{
  public:
	template <typename TValue> static std::uint64_t AppendCount(std::uint64_t hash, const std::vector<TValue>& values) noexcept
	{
		return Hash::ContinueFnv1a64Value(hash, static_cast<std::uint64_t>(values.size()));
	}

	static std::uint64_t AppendLightState(std::uint64_t hash, const DirectionalLight& light) noexcept
	{
		hash = LightingStateHash::AppendFloat3(hash, light.direction);
		hash = Hash::ContinueFnv1a64Value(hash, light.intensity);
		hash = LightingStateHash::AppendFloat3(hash, light.color);
		hash = Hash::ContinueFnv1a64Value(hash, light.angularDiameterRadians);
		return LightingStateHash::AppendBool(hash, light.castShadow);
	}

	static std::uint64_t AppendLightState(std::uint64_t hash, const PointLight& light) noexcept
	{
		hash = LightingStateHash::AppendFloat3(hash, light.position);
		hash = Hash::ContinueFnv1a64Value(hash, light.range);
		hash = LightingStateHash::AppendFloat3(hash, light.color);
		hash = Hash::ContinueFnv1a64Value(hash, light.intensity);
		hash = Hash::ContinueFnv1a64Value(hash, light.sourceRadius);
		return LightingStateHash::AppendBool(hash, light.castShadow);
	}

	static std::uint64_t AppendLightState(std::uint64_t hash, const SpotLight& light) noexcept
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

	static std::uint64_t AppendLightState(std::uint64_t hash, const RectLight& light) noexcept
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

	template <typename TLight>
	static std::uint64_t AppendLightsState(
	    std::uint64_t hash,
	    const RenderLightCollection<TLight>& lights) noexcept
	{
		hash = Hash::ContinueFnv1a64Value(hash, static_cast<std::uint64_t>(lights.size()));
		for (std::size_t index = 0; index < lights.size(); ++index)
		{
			const RenderObjectId object = lights.GetObject(index);
			hash = Hash::ContinueFnv1a64Value(hash, object.GetValue());
			hash = Hash::ContinueFnv1a64Value(hash, object.GetGeneration());
			hash = AppendLightState(hash, lights[index]);
		}
		return hash;
	}

	static std::uint64_t AppendMeshState(std::uint64_t hash, const MeshDraw& draw) noexcept
	{
		hash = LightingStateHash::AppendMatrix(hash, draw.Transform.WorldMatrix);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Material.Slot);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Skinning.SkeletonAssetId);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Skinning.JointMatrixOffset);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Morph.WeightOffset);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Morph.TargetCount);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Morph.VertexCount);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Geometry.MeshKind);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Source.MeshAssetId);
		hash = Hash::ContinueFnv1a64Value(hash, draw.Source.MeshGeneration);
		return Hash::ContinueFnv1a64Value(hash, draw.Geometry.Mesh.Value);
	}

	static std::uint64_t AppendMaterialState(std::uint64_t hash, const MaterialData& material) noexcept
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
		hash = Hash::ContinueFnv1a64Value(hash, material.gpuHandle.Index);
		return Hash::ContinueFnv1a64Value(hash, material.gpuHandle.Generation);
	}

	static std::uint64_t AppendSkyState(std::uint64_t hash, const RenderSkyData& sky) noexcept
	{
		hash = LightingStateHash::AppendBool(hash, sky.enabled);
		hash = LightingStateHash::AppendFloat3(hash, sky.color);
		hash = Hash::ContinueFnv1a64Value(hash, sky.intensity);
		hash = LightingStateHash::AppendBool(hash, sky.HasTexture());
		if (!sky.HasTexture())
		{
			return hash;
		}

		return Hash::ContinueFnv1a64Value(hash, sky.texture->ShaderResourceView.Value);
	}
};

std::uint64_t BuildLightingSceneInvalidationHash(const FrameContext& frame) noexcept
{
	std::uint64_t hash = Hash::kFnv64OffsetBasis;
	hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowsEnabled.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowNormalBias.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarRayTracedShadowMaxDistance.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarMaxDirectionalLights.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarMaxPointLights.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarMaxSpotLights.Get());
	hash = Hash::ContinueFnv1a64Value(hash, CVarMaxRectLights.Get());
	hash = LightingSceneStateOperations::AppendSkyState(hash, frame.sceneData.sky);
	hash = LightingSceneStateOperations::AppendLightsState(hash, frame.sceneData.directionalLights);
	hash = LightingSceneStateOperations::AppendLightsState(hash, frame.sceneData.pointLights);
	hash = LightingSceneStateOperations::AppendLightsState(hash, frame.sceneData.spotLights);
	hash = LightingSceneStateOperations::AppendLightsState(hash, frame.sceneData.rectLights);

	hash = LightingSceneStateOperations::AppendCount(hash, frame.sceneData.meshInstances);
	for (const MeshDraw& draw : frame.sceneData.meshInstances)
	{
		hash = LightingSceneStateOperations::AppendMeshState(hash, draw);
	}

	hash = LightingSceneStateOperations::AppendCount(hash, frame.sceneData.jointMatrices);
	for (const DirectX::XMFLOAT4X4& jointMatrix : frame.sceneData.jointMatrices)
	{
		hash = LightingStateHash::AppendMatrix(hash, jointMatrix);
	}

	hash = LightingSceneStateOperations::AppendCount(hash, frame.sceneData.morphWeights);
	for (float morphWeight : frame.sceneData.morphWeights)
	{
		hash = Hash::ContinueFnv1a64Value(hash, morphWeight);
	}

	hash = LightingSceneStateOperations::AppendCount(hash, frame.sceneData.materials);
	for (const MaterialData& material : frame.sceneData.materials)
	{
		hash = LightingSceneStateOperations::AppendMaterialState(hash, material);
	}

	hash = Hash::ContinueFnv1a64Value(hash, frame.sceneData.materialTextureTable.Binding.Table.Value);
	hash = Hash::ContinueFnv1a64Value(hash, frame.sceneData.materialTextureTable.DescriptorCount);
	hash = Hash::ContinueFnv1a64Value(hash, frame.sceneData.materialTextureTable.Generation);
	return Hash::FinalizeFnv1a64(hash);
}
