#include "PCH.h"
#include "Passes/Lighting/Restir/Indirect/RestirIndirectReservoirResources.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Resources/History/FrameHistory.h"

void CreateRestirIndirectHistoryResources(FrameGraphBuilder& builder, RenderViewportExtent extent, RenderFrameGraphResources& resources)
{
	resources.History.RestirIndirectReservoir = DeclareLightingReservoirHistory(builder, extent, "RestirIndirectReservoir");
}

RestirIndirectWorkingReservoirs CreateRestirIndirectWorkingReservoirs(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	const auto createReservoirTexture = [&](const char* name)
	{
		return builder.CreateTexture(FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, PixelFormat::R32G32B32A32_Float));
	};

	return RestirIndirectWorkingReservoirs{
	    .TemporalSample = createReservoirTexture("RestirIndirectTemporalReservoirSample"),
	    .TemporalWeight = createReservoirTexture("RestirIndirectTemporalReservoirWeight")};
}
