#include "../../PCH.h"
#include "Frame/Core/FrameSceneResources.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
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

	const FrameGraphTextureHandle backBuffer = builder.ImportTexture(
	    FrameGraphTextureDesc::CreateColor("BackBuffer", outputExtent.Width, outputExtent.Height, backBufferFormat),
	    ResourceState::Present);

	const FrameGraphTextureHandle exposure =
	    builder.CreateTexture(FrameGraphTextureDesc::CreateColor("Exposure", 1, 1, PixelFormat::R32G32B32A32_Float));
	const FrameGraphTextureHandle previousExposure = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("PreviousExposureHistory", 1, 1, PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentExposure = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor("CurrentExposureHistory", 1, 1, PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle previousDirectLightReservoirSample = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "PreviousDirectLightReservoirSample",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle previousDirectLightReservoirWeight = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "PreviousDirectLightReservoirWeight",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle previousDirectLightReservoirSurface = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "PreviousDirectLightReservoirSurface",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R16G16B16A16_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentDirectLightReservoirSample = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "CurrentDirectLightReservoirSample",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentDirectLightReservoirWeight = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "CurrentDirectLightReservoirWeight",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentDirectLightReservoirSurface = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "CurrentDirectLightReservoirSurface",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R16G16B16A16_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle previousReferenceLighting = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "PreviousReferenceLightingHistory",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const FrameGraphTextureHandle currentReferenceLighting = builder.ReservePersistentTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "CurrentReferenceLightingHistory",
	        renderExtent.Width,
	        renderExtent.Height,
	        PixelFormat::R32G32B32A32_Float),
	    ResourceState::ShaderResource);
	const auto reserveRestirIndirectHistory = [&](const char* name, PixelFormat format)
	{
		return builder.ReservePersistentTexture(
		    FrameGraphTextureDesc::CreateColor(name, renderExtent.Width, renderExtent.Height, format),
		    ResourceState::ShaderResource);
	};
	const FrameGraphTextureHandle previousRestirIndirectReservoirSample =
	    reserveRestirIndirectHistory("PreviousRestirIndirectReservoirSample", PixelFormat::R32G32B32A32_Float);
	const FrameGraphTextureHandle previousRestirIndirectReservoirWeight =
	    reserveRestirIndirectHistory("PreviousRestirIndirectReservoirWeight", PixelFormat::R32G32B32A32_Float);
	const FrameGraphTextureHandle previousRestirIndirectReservoirSurface =
	    reserveRestirIndirectHistory("PreviousRestirIndirectReservoirSurface", PixelFormat::R16G16B16A16_Float);
	const FrameGraphTextureHandle currentRestirIndirectReservoirSample =
	    reserveRestirIndirectHistory("CurrentRestirIndirectReservoirSample", PixelFormat::R32G32B32A32_Float);
	const FrameGraphTextureHandle currentRestirIndirectReservoirWeight =
	    reserveRestirIndirectHistory("CurrentRestirIndirectReservoirWeight", PixelFormat::R32G32B32A32_Float);
	const FrameGraphTextureHandle currentRestirIndirectReservoirSurface =
	    reserveRestirIndirectHistory("CurrentRestirIndirectReservoirSurface", PixelFormat::R16G16B16A16_Float);
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
	resources.History.PreviousExposure = previousExposure;
	resources.History.CurrentExposure = currentExposure;
	resources.History.PreviousDirectLightReservoirSample = previousDirectLightReservoirSample;
	resources.History.PreviousDirectLightReservoirWeight = previousDirectLightReservoirWeight;
	resources.History.PreviousDirectLightReservoirSurface = previousDirectLightReservoirSurface;
	resources.History.CurrentDirectLightReservoirSample = currentDirectLightReservoirSample;
	resources.History.CurrentDirectLightReservoirWeight = currentDirectLightReservoirWeight;
	resources.History.CurrentDirectLightReservoirSurface = currentDirectLightReservoirSurface;
	resources.History.PreviousReferenceLighting = previousReferenceLighting;
	resources.History.CurrentReferenceLighting = currentReferenceLighting;
	resources.History.PreviousRestirIndirectReservoirSample = previousRestirIndirectReservoirSample;
	resources.History.PreviousRestirIndirectReservoirWeight = previousRestirIndirectReservoirWeight;
	resources.History.PreviousRestirIndirectReservoirSurface = previousRestirIndirectReservoirSurface;
	resources.History.CurrentRestirIndirectReservoirSample = currentRestirIndirectReservoirSample;
	resources.History.CurrentRestirIndirectReservoirWeight = currentRestirIndirectReservoirWeight;
	resources.History.CurrentRestirIndirectReservoirSurface = currentRestirIndirectReservoirSurface;
	resources.ViewportProducts.SceneColor = sceneColor;
	resources.ViewportProducts.SceneDepth = sceneDepth;
	resources.ViewportProducts.FinalSceneColor = finalSceneColor;
	resources.ViewportProducts.Exposure = exposure;
}
