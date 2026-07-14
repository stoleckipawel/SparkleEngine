#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/PassRuntimeServices.h"

namespace DirectLightReservoirPassCommon
{
	void SetParameters(
	    DirectLightReservoirCommonParameters& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeServices& passRuntimeServices)
	{
		parameters.PerFrame = passRuntimeServices.PerFrame;
		parameters.PerView = viewData.perViewData;
		parameters.ViewLighting = frame.lighting.GetConstants();
	}
}
