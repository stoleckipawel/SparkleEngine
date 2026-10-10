#include "PCH.h"
#include "Passes/Lighting/Direct/DirectLightingResources.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/Lighting/LightingRenderTargets.h"
#include "Passes/Lighting/LightingTargetClear.h"
#include "Resources/History/FrameHistory.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include <array>

void CreateDirectLightingResources(FrameGraphBuilder& builder, RenderViewportExtent extent, RenderFrameGraphResources& resources)
{
	CreateDirectLightingRenderTargets(builder, extent, resources);
	const auto& lighting = resources.Transient.Lighting;
	const std::array targets{lighting.DirectDiffuse, lighting.DirectSpecular, lighting.DirectSubsurface};
	AddLightingTargetClearPass(builder, "DirectLightingTargetClear", extent, targets);
}

void CreateDirectLightReservoirResources(FrameGraphBuilder& builder, RenderViewportExtent extent, RenderFrameGraphResources& resources)
{
	resources.History.DirectLightReservoir = DeclareLightingReservoirHistory(builder, extent, "DirectLightReservoir");

	resources.Transient.DirectLightTemporalReservoirSample = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("DirectLightTemporalReservoirSample", extent.Width, extent.Height, PixelFormat::R32G32B32A32_Float));

	resources.Transient.DirectLightTemporalReservoirWeight = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor("DirectLightTemporalReservoirWeight", extent.Width, extent.Height, PixelFormat::R32G32B32A32_Float));
}
