#include "../../PCH.h"
#include "Frame/Graph/RenderFrameGraphResourceBindings.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Frame/Graph/RenderFrameGraphFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "Resources/History/FrameHistory.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RayTracing/RayTracingHitData.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"

class RenderSceneGpuResourceBindings final
{
public:
	template <typename TValue> static FrameGraphBufferHandle Declare(FrameGraphBuilder& builder, const char* name)
	{
		return builder.ReservePersistentBuffer(
		    FrameGraphBufferDesc::Create(name, sizeof(TValue), static_cast<std::uint32_t>(sizeof(TValue))),
		    ResourceState::ShaderResource);
	}

	static void Bind(FrameGraph& graph, FrameGraphBufferHandle handle, const RenderSceneGpuBufferBinding& buffer, const char* name) noexcept
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

RenderSceneGpuResources DeclareRenderSceneGpuResources(FrameGraphBuilder& builder)
{
	return RenderSceneGpuResources{
	    .Lighting =
	        RenderSceneGpuLightingResources{
	            .DirectionalLights = RenderSceneGpuResourceBindings::Declare<DirectionalLightGpuData>(builder, "DirectionalLights"),
	            .PointLights = RenderSceneGpuResourceBindings::Declare<PointLightGpuData>(builder, "PointLights"),
	            .SpotLights = RenderSceneGpuResourceBindings::Declare<SpotLightGpuData>(builder, "SpotLights"),
	            .RectLights = RenderSceneGpuResourceBindings::Declare<RectLightGpuData>(builder, "RectLights")},
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
    const RenderSceneGpuBindings& sceneGpuBindings) noexcept
{
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.Lighting.DirectionalLights,
	    sceneGpuBindings.Lighting.DirectionalLights,
	    "DirectionalLights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Lighting.PointLights, sceneGpuBindings.Lighting.PointLights, "PointLights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Lighting.SpotLights, sceneGpuBindings.Lighting.SpotLights, "SpotLights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Lighting.RectLights, sceneGpuBindings.Lighting.RectLights, "RectLights");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Geometry.MeshInstances, sceneGpuBindings.Geometry.MeshInstances, "MeshInstances");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.Geometry.MeshInstanceSlots,
	    sceneGpuBindings.Geometry.MeshInstanceSlots,
	    "MeshInstanceSlots");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Geometry.JointMatrices, sceneGpuBindings.Geometry.JointMatrices, "JointMatrices");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.Geometry.PreviousJointMatrices,
	    sceneGpuBindings.Geometry.PreviousJointMatrices,
	    "PreviousJointMatrices");
	RenderSceneGpuResourceBindings::Bind(graph, resources.Geometry.MorphWeights, sceneGpuBindings.Geometry.MorphWeights, "MorphWeights");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.Geometry.PreviousMorphWeights,
	    sceneGpuBindings.Geometry.PreviousMorphWeights,
	    "PreviousMorphWeights");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.Vertices,
	    sceneGpuBindings.RayTracing.Vertices,
	    "RayTracingHitVertices");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.SkinInfluences,
	    sceneGpuBindings.RayTracing.SkinInfluences,
	    "RayTracingHitSkinInfluences");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.MorphTargetDeltas,
	    sceneGpuBindings.RayTracing.MorphTargetDeltas,
	    "RayTracingHitMorphTargetDeltas");
	RenderSceneGpuResourceBindings::Bind(graph, resources.RayTracing.Indices, sceneGpuBindings.RayTracing.Indices, "RayTracingHitIndices");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.Instances,
	    sceneGpuBindings.RayTracing.Instances,
	    "RayTracingHitInstances");
	RenderSceneGpuResourceBindings::Bind(
	    graph,
	    resources.RayTracing.Materials,
	    sceneGpuBindings.RayTracing.Materials,
	    "RayTracingHitMaterials");
}

void CreateRenderFrameGraphResources(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    RenderFrameGraphResources& resources)
{
	const FrameGraphTextureHandle sceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneColor",
	        settings.RenderExtent.Width,
	        settings.RenderExtent.Height,
	        RenderFrameGraphFormats::SceneColor));

	const FrameGraphTextureHandle sceneDepth = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "SceneDepth",
	        settings.RenderExtent.Width,
	        settings.RenderExtent.Height,
	        RenderFrameGraphFormats::SceneDepth));

	FrameGraphTextureHandle backBuffer = FrameGraphTextureHandle::Invalid();
	if (settings.PresentationTarget == FramePresentationTarget::BackBuffer)
	{
		backBuffer = builder.ImportBackBuffer(
		    FrameGraphTextureDesc::CreateColor(
		        "BackBuffer",
		        settings.OutputExtent.Width,
		        settings.OutputExtent.Height,
		        settings.OutputFormat),
		    ResourceState::Present);
	}

	const FrameGraphTextureHandle exposure =
	    builder.CreateTexture(FrameGraphTextureDesc::CreateColor("Exposure", 1, 1, PixelFormat::R32G32B32A32_Float));
	const FrameGraphTextureHandle sky = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("Sky", 1, 1, PixelFormat::R8G8B8A8_UNorm),
	    ResourceState::ShaderResource);

	resources.Transient.Scene = SceneRenderTargets{.SceneColor = sceneColor, .SceneDepth = sceneDepth, .BackBuffer = backBuffer};
	resources.Transient.Exposure = exposure;
	resources.ImportedScene.Sky = sky;
	resources.ImportedScene.Scene = DeclareRenderSceneGpuResources(builder);
	resources.History = DeclareFrameHistoryResources(builder, settings.RenderExtent);
	resources.ViewportProducts.SceneDepth = sceneDepth;
}

FrameGraphTextureHandle CreateResolvedSceneColor(FrameGraphBuilder& builder, RenderViewportExtent outputExtent)
{
	return builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ResolvedSceneColor",
	        outputExtent.Width,
	        outputExtent.Height,
	        RenderFrameGraphFormats::SceneColor));
}
