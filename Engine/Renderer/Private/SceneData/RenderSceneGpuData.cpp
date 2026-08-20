#include "PCH.h"
#include "SceneData/RenderSceneGpuData.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "RayTracing/RayTracingHitData.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"
#include "ShaderData/MeshInstanceShaderData.h"

class RenderSceneGpuResourceBindings final
{
public:
	template <typename TValue> static FrameGraphBufferHandle Declare(FrameGraphBuilder& builder, const char* name)
	{
		return builder.ReservePersistentBuffer(
		    FrameGraphBufferDesc::Create(name, sizeof(TValue), static_cast<std::uint32_t>(sizeof(TValue))),
		    ResourceState::ShaderResource);
	}

	static void Bind(FrameGraph& graph, FrameGraphBufferHandle handle, const RenderSceneGpuBuffer& buffer, const char* name) noexcept
	{
		if (!buffer)
		{
			static const auto logger = Logging::GetOrCreateLogger("Renderer.RenderSceneGpuResourceBindings");
			Diagnostics::Fatal(logger, __FILE__, __LINE__, "Render-scene GPU buffer publication is incomplete.");
		}
		graph.BindPersistentBuffer(
		    handle,
		    buffer.Resource,
		    FrameGraphBufferDesc::Create(name, buffer.SizeInBytes, buffer.StrideInBytes),
		    ResourceState::ShaderResource);
	}
};

bool RenderSceneGpuBuffer::IsValid() const noexcept
{
	return Resource && SizeInBytes > 0 && StrideInBytes > 0;
}

RenderSceneGpuBuffer::operator bool() const noexcept
{
	return IsValid();
}

bool RenderSceneGpuGeometryData::HasMeshInstanceBuffers() const noexcept
{
	return MeshInstances.IsValid() && MeshInstanceSlots.IsValid();
}

bool RenderSceneGpuGeometryData::HasSkinningBuffers() const noexcept
{
	return JointMatrices.IsValid() && PreviousJointMatrices.IsValid();
}

bool RenderSceneGpuGeometryData::HasMorphingBuffers() const noexcept
{
	return MorphWeights.IsValid() && PreviousMorphWeights.IsValid();
}

bool RenderSceneGpuRayTracingData::HasCompleteBuffers() const noexcept
{
	return Vertices && SkinInfluences && MorphTargetDeltas && Indices && Instances && Materials;
}

RenderSceneGpuResources DeclareRenderSceneGpuResources(FrameGraphBuilder& builder)
{
	return RenderSceneGpuResources{
	    .Lighting =
	        RenderSceneGpuLightingResources{
	            .DirectionalLights =
	                RenderSceneGpuResourceBindings::Declare<DirectionalLightConstantBufferData>(builder, "DirectionalLights"),
	            .PointLights = RenderSceneGpuResourceBindings::Declare<PointLightConstantBufferData>(builder, "PointLights"),
	            .SpotLights = RenderSceneGpuResourceBindings::Declare<SpotLightConstantBufferData>(builder, "SpotLights"),
	            .RectLights = RenderSceneGpuResourceBindings::Declare<RectLightConstantBufferData>(builder, "RectLights")},
	    .Geometry =
	        RenderSceneGpuGeometryResources{
	            .MeshInstances = RenderSceneGpuResourceBindings::Declare<MeshInstanceData>(builder, "MeshInstances"),
	            .MeshInstanceSlots = RenderSceneGpuResourceBindings::Declare<std::uint32_t>(builder, "MeshInstanceSlots"),
	            .JointMatrices = RenderSceneGpuResourceBindings::Declare<JointMatrixData>(builder, "JointMatrices"),
	            .PreviousJointMatrices = RenderSceneGpuResourceBindings::Declare<JointMatrixData>(builder, "PreviousJointMatrices"),
	            .MorphWeights = RenderSceneGpuResourceBindings::Declare<float>(builder, "MorphWeights"),
	            .PreviousMorphWeights = RenderSceneGpuResourceBindings::Declare<float>(builder, "PreviousMorphWeights")},
	    .RayTracing = RenderSceneGpuRayTracingResources{
	        .Vertices = RenderSceneGpuResourceBindings::Declare<RayTracingHitVertex>(builder, "RayTracingHitVertices"),
	        .SkinInfluences = RenderSceneGpuResourceBindings::Declare<VertexSkinInfluenceData>(builder, "RayTracingHitSkinInfluences"),
	        .MorphTargetDeltas = RenderSceneGpuResourceBindings::Declare<MorphTargetDeltaData>(builder, "RayTracingHitMorphTargetDeltas"),
	        .Indices = RenderSceneGpuResourceBindings::Declare<std::uint32_t>(builder, "RayTracingHitIndices"),
	        .Instances = RenderSceneGpuResourceBindings::Declare<RayTracingHitInstance>(builder, "RayTracingHitInstances"),
	        .Materials = RenderSceneGpuResourceBindings::Declare<RayTracingHitMaterial>(builder, "RayTracingHitMaterials")}};
}

void BindRenderSceneGpuResources(
    FrameGraph& graph,
    const RenderSceneGpuResources& resources,
    const RenderSceneGpuData& sceneGpuData) noexcept
{
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.Lighting.DirectionalLights,
	    sceneGpuData.Lighting.DirectionalLights,
	    "DirectionalLights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Lighting.PointLights, sceneGpuData.Lighting.PointLights, "PointLights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Lighting.SpotLights, sceneGpuData.Lighting.SpotLights, "SpotLights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Lighting.RectLights, sceneGpuData.Lighting.RectLights, "RectLights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Geometry.MeshInstances, sceneGpuData.Geometry.MeshInstances, "MeshInstances");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.Geometry.MeshInstanceSlots,
	    sceneGpuData.Geometry.MeshInstanceSlots,
	    "MeshInstanceSlots");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Geometry.JointMatrices, sceneGpuData.Geometry.JointMatrices, "JointMatrices");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.Geometry.PreviousJointMatrices,
	    sceneGpuData.Geometry.PreviousJointMatrices,
	    "PreviousJointMatrices");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Geometry.MorphWeights, sceneGpuData.Geometry.MorphWeights, "MorphWeights");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.Geometry.PreviousMorphWeights,
	    sceneGpuData.Geometry.PreviousMorphWeights,
	    "PreviousMorphWeights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.RayTracing.Vertices, sceneGpuData.RayTracing.Vertices, "RayTracingHitVertices");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.SkinInfluences,
	    sceneGpuData.RayTracing.SkinInfluences,
	    "RayTracingHitSkinInfluences");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.MorphTargetDeltas,
	    sceneGpuData.RayTracing.MorphTargetDeltas,
	    "RayTracingHitMorphTargetDeltas");
	RenderSceneGpuResourceBindings::Bind(graph, resources.RayTracing.Indices, sceneGpuData.RayTracing.Indices, "RayTracingHitIndices");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.Instances,
	    sceneGpuData.RayTracing.Instances,
	    "RayTracingHitInstances");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.Materials,
	    sceneGpuData.RayTracing.Materials,
	    "RayTracingHitMaterials");
}
