#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"

namespace DirectLightReservoirPassCommon
{
	void DeclareResources(
	    FrameGraphBuilder& builder,
	    FrameGraphTextureHandle sceneDepth,
	    const GBufferRenderTargets& gbuffer,
	    DirectLightReservoirCommonParameters& parameters)
	{
		parameters.GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
		parameters.GBufferNormal = builder.CreateSRV(gbuffer.Normal);
		parameters.GBufferMaterial = builder.CreateSRV(gbuffer.Material);
		parameters.GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
		parameters.SceneDepth = builder.CreateSRV(sceneDepth);
	}

	void SetParameters(
	    DirectLightReservoirCommonParameters& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices)
	{
		parameters.PerFrame = passRuntimeServices.PerFrame;
		parameters.PerView = viewData.perViewData;
		parameters.ViewLighting = frame.lighting.GetConstants();
		parameters.DirectionalLights = frame.lighting.GetDirectionalLightsShaderResourceView();
		parameters.PointLights = frame.lighting.GetPointLightsShaderResourceView();
		parameters.SpotLights = frame.lighting.GetSpotLightsShaderResourceView();
		parameters.RectLights = frame.lighting.GetRectLightsShaderResourceView();
	}
}
