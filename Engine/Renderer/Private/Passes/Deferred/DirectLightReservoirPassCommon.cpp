#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/PassRuntimeContext.h"

namespace DirectLightReservoirPassCommon
{
	void SetParameters(
	    DirectLightReservoirCommonParameters& parameters,
	    const FrameContext& frame,
	    const RenderViewData& viewData,
	    const PassRuntimeContext& passRuntimeContext)
	{
		parameters.PerFrame = passRuntimeContext.PerFrame;
		parameters.PerView = viewData.perViewData;
		parameters.ViewLighting = frame.sceneGpuData->Lighting.Constants;
	}
}
