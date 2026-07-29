#include "PCH.h"
#include "SceneData/GpuScene/RenderGpuScenePayloadBuilder.h"

#include "Lighting/LightingCVars.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Meshes/GpuMesh.h"
#include "Meshes/GpuMeshCache.h"
#include "Meshes/GpuMeshPreparation.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <format>
#include <limits>
#include <unordered_map>
#include <unordered_set>

static const auto g_renderGpuScenePayloadBuilderLogger = Logging::GetOrCreateLogger("Renderer.RenderGpuScenePayloadBuilder");

class RenderGpuLightingContract final
{
  public:
	static void ValidateCount(std::string_view lightKind, std::size_t count, std::uint32_t configuredLimit)
	{
		if (count > configuredLimit)
		{
			Diagnostics::Fatal(
			    g_renderGpuScenePayloadBuilderLogger,
			    __FILE__,
			    __LINE__,
			    std::format("Scene contains {} {} lights, exceeding the configured limit of {}.", count, lightKind, configuredLimit));
		}
	}
};

struct RenderGpuMeshHitDataOffsets final
{
	std::uint32_t FirstVertex = 0;
	std::uint32_t FirstIndex = 0;
	std::uint32_t FirstMorphTargetDelta = 0;
	std::uint32_t VertexCount = 0;
	std::uint32_t IndexCount = 0;
};

struct RenderGpuScenePayloadBuilder::RayTracingBuildState final
{
	std::unordered_map<const GpuMesh*, RenderGpuMeshHitDataOffsets> MeshOffsets;

	RenderGpuMeshHitDataOffsets ResolveMeshOffsets(const GpuMesh& mesh, RenderGpuRayTracingPayloads& payloads);
};

RenderGpuMeshHitDataOffsets RenderGpuScenePayloadBuilder::RayTracingBuildState::ResolveMeshOffsets(
    const GpuMesh& mesh,
    RenderGpuRayTracingPayloads& payloads)
{
	if (const auto existing = MeshOffsets.find(&mesh); existing != MeshOffsets.end())
	{
		return existing->second;
	}

	const std::span<const RayTracingHitVertex> meshVertices = mesh.GetRayTracingHitVertices();
	const std::span<const std::uint32_t> meshIndices = mesh.GetRayTracingHitIndices();
	const std::span<const VertexSkinInfluence> skinInfluences = mesh.GetSkinInfluences();
	const std::span<const MorphTargetDeltaData> morphTargetDeltas = mesh.GetMorphTargetDeltas();
	constexpr std::size_t maximumPayloadElementCount = (std::numeric_limits<std::uint32_t>::max)();
	if (!mesh.HasRayTracingHitData() || meshVertices.empty() || meshIndices.size() < 3u || meshIndices.size() % 3u != 0u ||
	    (mesh.HasSkinInfluences() && skinInfluences.size() != meshVertices.size()) ||
	    payloads.Vertices.size() > maximumPayloadElementCount || payloads.Indices.size() > maximumPayloadElementCount ||
	    payloads.MorphTargetDeltas.size() > maximumPayloadElementCount ||
	    meshVertices.size() > maximumPayloadElementCount - payloads.Vertices.size() ||
	    meshIndices.size() > maximumPayloadElementCount - payloads.Indices.size() ||
	    morphTargetDeltas.size() > maximumPayloadElementCount - payloads.MorphTargetDeltas.size())
	{
		Diagnostics::Fatal(g_renderGpuScenePayloadBuilderLogger, __FILE__, __LINE__, "Ray-tracing work references invalid mesh hit data.");
	}

	const RenderGpuMeshHitDataOffsets offsets{
	    .FirstVertex = static_cast<std::uint32_t>(payloads.Vertices.size()),
	    .FirstIndex = static_cast<std::uint32_t>(payloads.Indices.size()),
	    .FirstMorphTargetDelta = static_cast<std::uint32_t>(payloads.MorphTargetDeltas.size()),
	    .VertexCount = static_cast<std::uint32_t>(meshVertices.size()),
	    .IndexCount = static_cast<std::uint32_t>(meshIndices.size())};
	payloads.Vertices.insert(payloads.Vertices.end(), meshVertices.begin(), meshVertices.end());
	if (mesh.HasSkinInfluences())
	{
		for (const VertexSkinInfluence& influence : skinInfluences)
		{
			payloads.SkinInfluences.push_back(GpuMeshPreparation::ConvertSkinInfluence(influence));
		}
	}
	else
	{
		payloads.SkinInfluences.resize(payloads.Vertices.size());
	}
	payloads.Indices.insert(payloads.Indices.end(), meshIndices.begin(), meshIndices.end());
	payloads.MorphTargetDeltas.insert(payloads.MorphTargetDeltas.end(), morphTargetDeltas.begin(), morphTargetDeltas.end());
	MeshOffsets.emplace(&mesh, offsets);
	return offsets;
}

void RenderGpuScenePayloadBuilder::BuildLighting(const RenderSceneData& sceneData, RenderGpuLightingPayloads& payloads)
{
	const std::size_t directionalLightCount = sceneData.directionalLights.size();
	const std::size_t pointLightCount = sceneData.pointLights.size();
	const std::size_t spotLightCount = sceneData.spotLights.size();
	const std::size_t rectLightCount = sceneData.rectLights.size();
	RenderGpuLightingContract::ValidateCount("directional", directionalLightCount, CVarMaxDirectionalLights.Get());
	RenderGpuLightingContract::ValidateCount("point", pointLightCount, CVarMaxPointLights.Get());
	RenderGpuLightingContract::ValidateCount("spot", spotLightCount, CVarMaxSpotLights.Get());
	RenderGpuLightingContract::ValidateCount("rect", rectLightCount, CVarMaxRectLights.Get());

	payloads.DirectionalLights.clear();
	payloads.PointLights.clear();
	payloads.SpotLights.clear();
	payloads.RectLights.clear();

	payloads.Constants = ViewLightingData{
	    .DirectionalLightCount = static_cast<std::uint32_t>(directionalLightCount),
	    .PointLightCount = static_cast<std::uint32_t>(pointLightCount),
	    .SpotLightCount = static_cast<std::uint32_t>(spotLightCount),
	    .RectLightCount = static_cast<std::uint32_t>(rectLightCount)};
	payloads.DirectionalLights.reserve(directionalLightCount);
	payloads.PointLights.reserve(pointLightCount);
	payloads.SpotLights.reserve(spotLightCount);
	payloads.RectLights.reserve(rectLightCount);

	for (std::size_t lightIndex = 0; lightIndex < directionalLightCount; ++lightIndex)
	{
		const DirectionalLight& light = sceneData.directionalLights[lightIndex];
		payloads.DirectionalLights.push_back(
		    DirectionalLightConstantBufferData{
		        .Direction = {light.direction.x, light.direction.y, light.direction.z},
		        .Illuminance = light.illuminance,
		        .Color = {light.color.x, light.color.y, light.color.z},
		        .AngularSizeRadians = light.angularSizeRadians,
		        .CastShadow = light.castShadow ? 1u : 0u});
	}

	for (std::size_t lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
	{
		const PointLight& light = sceneData.pointLights[lightIndex];
		payloads.PointLights.push_back(
		    PointLightConstantBufferData{
		        .Position = {light.position.x, light.position.y, light.position.z},
		        .Range = light.range,
		        .Color = {light.color.x, light.color.y, light.color.z},
		        .LuminousIntensity = light.luminousIntensity,
		        .DistanceAttenuationCoefficients = light.distanceAttenuationCoefficients,
		        .Radius = light.radius,
		        .CastShadow = light.castShadow ? 1u : 0u});
	}

	for (std::size_t lightIndex = 0; lightIndex < spotLightCount; ++lightIndex)
	{
		const SpotLight& light = sceneData.spotLights[lightIndex];
		payloads.SpotLights.push_back(
		    SpotLightConstantBufferData{
		        .Position = {light.position.x, light.position.y, light.position.z},
		        .Range = light.range,
		        .Direction = {light.direction.x, light.direction.y, light.direction.z},
		        .InnerAngleCosine = light.innerAngleCosine,
		        .Color = {light.color.x, light.color.y, light.color.z},
		        .LuminousIntensity = light.luminousIntensity,
		        .DistanceAttenuationCoefficients = light.distanceAttenuationCoefficients,
		        .Radius = light.radius,
		        .OuterAngleCosine = light.outerAngleCosine,
		        .CastShadow = light.castShadow ? 1u : 0u});
	}

	for (std::size_t lightIndex = 0; lightIndex < rectLightCount; ++lightIndex)
	{
		const RectLight& light = sceneData.rectLights[lightIndex];
		payloads.RectLights.push_back(
		    RectLightConstantBufferData{
		        .Position = {light.position.x, light.position.y, light.position.z},
		        .Width = light.width,
		        .Direction = {light.direction.x, light.direction.y, light.direction.z},
		        .Height = light.height,
		        .Tangent = {light.tangent.x, light.tangent.y, light.tangent.z},
		        .Luminance = light.luminance,
		        .Color = {light.color.x, light.color.y, light.color.z},
		        .CastShadow = light.castShadow ? 1u : 0u});
	}

}

void RenderGpuScenePayloadBuilder::AppendRayTracingMaterials(const RenderSceneData& sceneData, RenderGpuRayTracingPayloads& payloads)
{
	payloads.Materials.reserve(sceneData.materials.size());
	for (const MaterialData& material : sceneData.materials)
	{
		payloads.Materials.push_back(
		    RayTracingHitMaterial{
		        .BaseColor = material.baseColor,
		        .EmissiveColor = material.emissiveColor,
		        .Metallic = material.metallic,
		        .Roughness = material.roughness,
		        .F0 = material.f0,
		        .AlphaCutoff = material.alphaCutoff,
		        .AlphaMode = material.alphaMode,
		        .TextureFlags = material.textureFlags,
		        .SubsurfaceColor = material.subsurfaceColor,
		        .SubsurfaceStrength = material.subsurfaceStrength,
		        .Flags = BuildMaterialFlags(material),
		        .TextureIndices0 =
		            DirectX::XMUINT4{
		                material.materialTextureIndices[MaterialTextureSlots::BaseColor],
		                material.materialTextureIndices[MaterialTextureSlots::Normal],
		                material.materialTextureIndices[MaterialTextureSlots::Roughness],
		                material.materialTextureIndices[MaterialTextureSlots::Metallic]},
		        .TextureIndices1 = DirectX::XMUINT4{
		            material.materialTextureIndices[MaterialTextureSlots::Occlusion],
		            material.materialTextureIndices[MaterialTextureSlots::Emissive],
		            material.materialTextureIndices[MaterialTextureSlots::SubsurfaceColor],
		            material.materialTextureIndices[MaterialTextureSlots::SubsurfaceStrength]}});
	}
}

void RenderGpuScenePayloadBuilder::PrepareRayTracingInstances(
    const RenderSceneData& sceneData,
    RenderGpuRayTracingPayloads& payloads,
    RayTracingBuildState& state)
{
	std::uint32_t instanceCapacity = 1;
	std::unordered_set<std::uint32_t> occupiedGpuSceneSlots;
	for (const RenderRayTracingBlasInput& input : sceneData.rayTracingWork.BlasInputs)
	{
		if (input.GpuSceneSlot == (std::numeric_limits<std::uint32_t>::max)() || !occupiedGpuSceneSlots.insert(input.GpuSceneSlot).second)
		{
			Diagnostics::Fatal(
			    g_renderGpuScenePayloadBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "Ray-tracing work contains an invalid or duplicate GPU-scene slot.");
		}
		instanceCapacity = std::max(instanceCapacity, input.GpuSceneSlot + 1u);
	}

	payloads.Instances.resize(instanceCapacity);
	state.MeshOffsets.reserve(sceneData.rayTracingWork.BlasInputs.size());
}

void RenderGpuScenePayloadBuilder::AppendRayTracingInstance(
    const RenderRayTracingBlasInput& input,
    const RenderSceneData& sceneData,
    const GpuMeshCache& meshes,
    RenderGpuRayTracingPayloads& payloads,
    RayTracingBuildState& state)
{
	if (input.MeshInstanceIndex >= sceneData.meshInstances.size())
	{
		Diagnostics::Fatal(
		    g_renderGpuScenePayloadBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing work references a mesh instance outside the render scene.");
	}
	const MeshDraw& draw = sceneData.meshInstances[input.MeshInstanceIndex];
	if (draw.Material.Slot >= sceneData.materials.size())
	{
		Diagnostics::Fatal(
		    g_renderGpuScenePayloadBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing work references a material outside the render scene.");
	}

	const GpuMesh* gpuMesh = meshes.Resolve(draw.Geometry.Mesh);
	const bool missingSkinning =
	    draw.Geometry.MeshKind == RenderMeshKind::Skeletal &&
	    (draw.Skinning.JointMatrixOffset == kInvalidMeshInstanceJointMatrixOffset || gpuMesh == nullptr || !gpuMesh->HasSkinInfluences());
	if (gpuMesh == nullptr || missingSkinning)
	{
		Diagnostics::Fatal(
		    g_renderGpuScenePayloadBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing work references incomplete mesh or skinning data.");
	}

	const MaterialData& material = sceneData.materials[draw.Material.Slot];
	const RenderGpuMeshHitDataOffsets offsets = state.ResolveMeshOffsets(*gpuMesh, payloads);
	RayTracingHitInstance& instance = payloads.Instances[input.GpuSceneSlot];
	instance = RayTracingHitInstance{
	    .FirstVertex = offsets.FirstVertex,
	    .FirstIndex = offsets.FirstIndex,
	    .VertexCount = offsets.VertexCount,
	    .IndexCount = offsets.IndexCount,
	    .MaterialSlot = draw.Material.Slot,
	    .Flags = RayTracingHitData::InstanceFlag_Valid | (material.alphaMode == 0 ? RayTracingHitData::InstanceFlag_Opaque : 0u) |
	             (draw.Geometry.MeshKind == RenderMeshKind::Static ? RayTracingHitData::InstanceFlag_StaticMesh : 0u) |
	             (material.doubleSided ? RayTracingHitData::InstanceFlag_TwoSided : 0u),
	    .GeometryFlags = BuildGeometryFlags(draw, material),
	    .RejectionReason = RayTracingHitData::Reason_None,
	    .AlphaMode = material.alphaMode,
	    .MaterialTextureFlags = material.textureFlags};
	instance.MorphTargetDeltaOffset = offsets.FirstMorphTargetDelta;
}

void RenderGpuScenePayloadBuilder::BuildRayTracing(
    const RenderSceneData& sceneData,
    const GpuMeshCache& meshes,
    RenderGpuRayTracingPayloads& payloads)
{
	ClearRayTracingPayloads(payloads);

	const RenderRayTracingWorkPlan& work = sceneData.rayTracingWork;
	if (work.BlasInputs.empty())
	{
		return;
	}
	if (sceneData.materials.empty())
	{
		Diagnostics::Fatal(
		    g_renderGpuScenePayloadBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing work exists without any material records.");
	}

	AppendRayTracingMaterials(sceneData, payloads);

	RayTracingBuildState state;
	PrepareRayTracingInstances(sceneData, payloads, state);
	for (const RenderRayTracingBlasInput& input : work.BlasInputs)
	{
		AppendRayTracingInstance(input, sceneData, meshes, payloads, state);
	}

	if (payloads.Vertices.empty() || payloads.Indices.empty())
	{
		Diagnostics::Fatal(g_renderGpuScenePayloadBuilderLogger, __FILE__, __LINE__, "Ray-tracing work produced no geometry payload.");
	}
	payloads.InstanceCount = static_cast<std::uint32_t>(payloads.Instances.size());
	payloads.MaterialCount = static_cast<std::uint32_t>(payloads.Materials.size());
}

std::uint32_t RenderGpuScenePayloadBuilder::BuildMaterialFlags(const MaterialData& material) noexcept
{
	std::uint32_t flags = material.doubleSided ? RayTracingHitData::MaterialFlag_DoubleSided : 0u;
	if (material.alphaMode == 0)
	{
		flags |= RayTracingHitData::MaterialFlag_Opaque;
	}
	else if (material.alphaMode == 1)
	{
		flags |= RayTracingHitData::MaterialFlag_AlphaTested;
	}
	else if (material.alphaMode == 2)
	{
		flags |= RayTracingHitData::MaterialFlag_AlphaBlended;
	}
	if (material.textureFlags != 0)
	{
		flags |= RayTracingHitData::MaterialFlag_Textured;
	}
	return flags;
}

std::uint32_t RenderGpuScenePayloadBuilder::BuildGeometryFlags(const MeshDraw& draw, const MaterialData& material) noexcept
{
	std::uint32_t flags = draw.Geometry.MeshKind == RenderMeshKind::Skeletal ? RayTracingHitData::GeometryFlag_SkinnedMesh
	                                                                         : RayTracingHitData::GeometryFlag_StaticMesh;
	if (material.alphaMode == 1)
	{
		flags |= RayTracingHitData::GeometryFlag_AlphaTested;
	}
	else if (material.alphaMode == 2)
	{
		flags |= RayTracingHitData::GeometryFlag_AlphaBlended;
	}
	if (material.textureFlags != 0)
	{
		flags |= RayTracingHitData::GeometryFlag_TexturedMaterial;
	}
	if (material.doubleSided)
	{
		flags |= RayTracingHitData::GeometryFlag_DoubleSided;
	}
	return flags;
}

void RenderGpuScenePayloadBuilder::ClearRayTracingPayloads(RenderGpuRayTracingPayloads& payloads) noexcept
{
	payloads.Vertices.clear();
	payloads.SkinInfluences.clear();
	payloads.MorphTargetDeltas.clear();
	payloads.Indices.clear();
	payloads.Instances.clear();
	payloads.Materials.clear();
	payloads.InstanceCount = 0u;
	payloads.MaterialCount = 0u;
}
