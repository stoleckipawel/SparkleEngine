#include "../../../PCH.h"
#include "Passes/Lighting/Restir/RestirIndirectReservoirResources.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

RestirIndirectWorkingReservoirs CreateRestirIndirectWorkingReservoirs(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	const auto createReservoirTexture = [&](const char* name)
	{
		return builder.CreateTexture(
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, PixelFormat::R32G32B32A32_Float));
	};
	return RestirIndirectWorkingReservoirs{
	    .TemporalSample = createReservoirTexture("RestirIndirectTemporalReservoirSample"),
	    .TemporalWeight = createReservoirTexture("RestirIndirectTemporalReservoirWeight")};
}
