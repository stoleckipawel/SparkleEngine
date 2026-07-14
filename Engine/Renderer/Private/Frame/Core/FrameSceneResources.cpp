#include "../../PCH.h"
#include "Frame/Core/FrameSceneResources.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Resources/History/FrameHistory.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RayTracing/RayTracingHitData.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

namespace
{
	template <typename TValue> FrameGraphBufferHandle ReserveExternalBuffer(FrameGraphBuilder& builder, const char* name)
	{
		return builder.ReservePersistentBuffer(
		    FrameGraphBufferDesc::Create(name, sizeof(TValue), static_cast<std::uint32_t>(sizeof(TValue))),
		    ResourceState::ShaderResource);
	}
}

void CreateFrameSceneResources(
    FrameGraphBuilder& builder,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    PixelFormat backBufferFormat,
    FrameAssemblyResourceLayout& resources)
{
	const FrameGraphTextureHandle sceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("SceneColor", renderExtent.Width, renderExtent.Height, FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle sceneDepth = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("SceneDepth", renderExtent.Width, renderExtent.Height, FrameRenderFormats::SceneDepth));

	const FrameGraphTextureHandle finalSceneColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("FinalSceneColor", outputExtent.Width, outputExtent.Height, FrameRenderFormats::SceneColor));

	const FrameGraphTextureHandle backBuffer = builder.ImportBackBuffer(
	    FrameGraphTextureDesc::CreateColor("BackBuffer", outputExtent.Width, outputExtent.Height, backBufferFormat),
	    ResourceState::Present);

	const FrameGraphTextureHandle exposure =
	    builder.CreateTexture(FrameGraphTextureDesc::CreateColor("Exposure", 1, 1, PixelFormat::R32G32B32A32_Float));
	const FrameGraphTextureHandle sky = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("Sky", 1, 1, PixelFormat::R8G8B8A8_UNorm),
	    ResourceState::ShaderResource);

	resources.Transient.Scene = SceneRenderTargets{
	    .SceneColor = sceneColor,
	    .SceneDepth = sceneDepth,
	    .FinalSceneColor = finalSceneColor,
	    .BackBuffer = backBuffer};
	resources.Transient.Exposure = exposure;
	resources.External.Sky = sky;
	resources.External.DirectionalLights = ReserveExternalBuffer<DirectionalLightConstantBufferData>(builder, "DirectionalLights");
	resources.External.PointLights = ReserveExternalBuffer<PointLightConstantBufferData>(builder, "PointLights");
	resources.External.SpotLights = ReserveExternalBuffer<SpotLightConstantBufferData>(builder, "SpotLights");
	resources.External.RectLights = ReserveExternalBuffer<RectLightConstantBufferData>(builder, "RectLights");
	resources.External.MeshInstances = ReserveExternalBuffer<MeshInstanceData>(builder, "MeshInstances");
	resources.External.RayTracingHitVertices = ReserveExternalBuffer<RayTracingHitVertex>(builder, "RayTracingHitVertices");
	resources.External.RayTracingHitSkinInfluences = ReserveExternalBuffer<VertexSkinInfluenceData>(builder, "RayTracingHitSkinInfluences");
	resources.External.RayTracingHitIndices = ReserveExternalBuffer<std::uint32_t>(builder, "RayTracingHitIndices");
	resources.External.RayTracingHitInstances = ReserveExternalBuffer<RayTracingHitInstance>(builder, "RayTracingHitInstances");
	resources.External.RayTracingHitMaterials = ReserveExternalBuffer<RayTracingHitMaterial>(builder, "RayTracingHitMaterials");
	resources.External.JointMatrices = ReserveExternalBuffer<JointMatrixData>(builder, "JointMatrices");
	resources.External.PreviousJointMatrices = ReserveExternalBuffer<JointMatrixData>(builder, "PreviousJointMatrices");
	resources.History = DeclareFrameHistoryResources(builder, renderExtent);
	resources.ViewportProducts.SceneColor = sceneColor;
	resources.ViewportProducts.SceneDepth = sceneDepth;
	resources.ViewportProducts.FinalSceneColor = finalSceneColor;
	resources.ViewportProducts.Exposure = exposure;
}
