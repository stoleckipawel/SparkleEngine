#include "PCH.h"
#include "Scene/GpuScene/RenderSceneFrameGraphResources.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphBufferDesc.h"
#include "RayTracing/RayTracingHitData.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"

template <typename TValue> static FrameGraphBufferHandle DeclareRenderSceneGpuBuffer(FrameGraphBuilder& builder, const char* name)
{
	return builder.ReservePersistentBuffer(
	    FrameGraphBufferDesc::Create(name, sizeof(TValue), static_cast<std::uint32_t>(sizeof(TValue))),
	    ResourceState::ShaderResource);
}

static void BindRenderSceneGpuBuffer(
    FrameGraph& graph,
    FrameGraphBufferHandle handle,
    const RenderSceneGpuBufferBinding& buffer,
    const char* name) noexcept
{
	if (!buffer)
	{
		static const auto logger = Logging::GetOrCreateLogger("Renderer.RenderSceneGpuResources");
		Diagnostics::Fatal(logger, __FILE__, __LINE__, "Render-scene GPU buffer publication is incomplete.");
	}

	graph.BindPersistentBuffer(
	    handle,
	    buffer.Resource,
	    FrameGraphBufferDesc::Create(name, buffer.SizeInBytes, buffer.StrideInBytes),
	    ResourceState::ShaderResource);
}

RenderSceneGpuResources DeclareRenderSceneGpuResources(FrameGraphBuilder& builder)
{
	return RenderSceneGpuResources{
	    .Lighting =
	        RenderSceneGpuLightingResources{
	            .DirectionalLights = DeclareRenderSceneGpuBuffer<DirectionalLightGpuData>(builder, "DirectionalLights"),
	            .PointLights = DeclareRenderSceneGpuBuffer<PointLightGpuData>(builder, "PointLights"),
	            .SpotLights = DeclareRenderSceneGpuBuffer<SpotLightGpuData>(builder, "SpotLights"),
	            .RectLights = DeclareRenderSceneGpuBuffer<RectLightGpuData>(builder, "RectLights")},
	    .Geometry =
	        RenderSceneGpuGeometryResources{
	            .MeshInstances = DeclareRenderSceneGpuBuffer<MeshInstanceData>(builder, "MeshInstances"),
	            .MeshInstanceSlots = DeclareRenderSceneGpuBuffer<std::uint32_t>(builder, "MeshInstanceSlots"),
	            .JointMatrices = DeclareRenderSceneGpuBuffer<JointMatrixData>(builder, "JointMatrices"),
	            .PreviousJointMatrices = DeclareRenderSceneGpuBuffer<JointMatrixData>(builder, "PreviousJointMatrices"),
	            .MorphWeights = DeclareRenderSceneGpuBuffer<float>(builder, "MorphWeights"),
	            .PreviousMorphWeights = DeclareRenderSceneGpuBuffer<float>(builder, "PreviousMorphWeights")},
	    .RayTracing = RenderSceneGpuRayTracingResources{
	        .Vertices = DeclareRenderSceneGpuBuffer<RayTracingHitVertex>(builder, "RayTracingHitVertices"),
	        .SkinInfluences = DeclareRenderSceneGpuBuffer<VertexSkinInfluenceData>(builder, "RayTracingHitSkinInfluences"),
	        .MorphTargetDeltas = DeclareRenderSceneGpuBuffer<MorphTargetDeltaData>(builder, "RayTracingHitMorphTargetDeltas"),
	        .Indices = DeclareRenderSceneGpuBuffer<std::uint32_t>(builder, "RayTracingHitIndices"),
	        .Instances = DeclareRenderSceneGpuBuffer<RayTracingHitInstance>(builder, "RayTracingHitInstances"),
	        .Materials = DeclareRenderSceneGpuBuffer<RayTracingHitMaterial>(builder, "RayTracingHitMaterials")}};
}

void BindRenderSceneGpuResources(
    FrameGraph& graph,
    const RenderSceneGpuResources& resources,
    const RenderSceneGpuBindings& sceneGpuBindings) noexcept
{
	BindRenderSceneGpuBuffer(graph, resources.Lighting.DirectionalLights, sceneGpuBindings.Lighting.DirectionalLights, "DirectionalLights");
	BindRenderSceneGpuBuffer(graph, resources.Lighting.PointLights, sceneGpuBindings.Lighting.PointLights, "PointLights");
	BindRenderSceneGpuBuffer(graph, resources.Lighting.SpotLights, sceneGpuBindings.Lighting.SpotLights, "SpotLights");
	BindRenderSceneGpuBuffer(graph, resources.Lighting.RectLights, sceneGpuBindings.Lighting.RectLights, "RectLights");

	BindRenderSceneGpuBuffer(graph, resources.Geometry.MeshInstances, sceneGpuBindings.Geometry.MeshInstances, "MeshInstances");
	BindRenderSceneGpuBuffer(graph, resources.Geometry.MeshInstanceSlots, sceneGpuBindings.Geometry.MeshInstanceSlots, "MeshInstanceSlots");
	BindRenderSceneGpuBuffer(graph, resources.Geometry.JointMatrices, sceneGpuBindings.Geometry.JointMatrices, "JointMatrices");

	BindRenderSceneGpuBuffer(
	    graph,
	    resources.Geometry.PreviousJointMatrices,
	    sceneGpuBindings.Geometry.PreviousJointMatrices,
	    "PreviousJointMatrices");

	BindRenderSceneGpuBuffer(graph, resources.Geometry.MorphWeights, sceneGpuBindings.Geometry.MorphWeights, "MorphWeights");

	BindRenderSceneGpuBuffer(
	    graph,
	    resources.Geometry.PreviousMorphWeights,
	    sceneGpuBindings.Geometry.PreviousMorphWeights,
	    "PreviousMorphWeights");

	BindRenderSceneGpuBuffer(graph, resources.RayTracing.Vertices, sceneGpuBindings.RayTracing.Vertices, "RayTracingHitVertices");

	BindRenderSceneGpuBuffer(
	    graph,
	    resources.RayTracing.SkinInfluences,
	    sceneGpuBindings.RayTracing.SkinInfluences,
	    "RayTracingHitSkinInfluences");

	BindRenderSceneGpuBuffer(
	    graph,
	    resources.RayTracing.MorphTargetDeltas,
	    sceneGpuBindings.RayTracing.MorphTargetDeltas,
	    "RayTracingHitMorphTargetDeltas");

	BindRenderSceneGpuBuffer(graph, resources.RayTracing.Indices, sceneGpuBindings.RayTracing.Indices, "RayTracingHitIndices");
	BindRenderSceneGpuBuffer(graph, resources.RayTracing.Instances, sceneGpuBindings.RayTracing.Instances, "RayTracingHitInstances");
	BindRenderSceneGpuBuffer(graph, resources.RayTracing.Materials, sceneGpuBindings.RayTracing.Materials, "RayTracingHitMaterials");
}
