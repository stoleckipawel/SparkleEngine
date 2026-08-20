#include "../../PCH.h"
#include "Passes/Deferred/DirectLightReservoirPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "View/RenderView.h"
#include "FrameGraph/PassRuntimeContext.h"

namespace DirectLightReservoirPassCommon
{
	void SetParameters(
	    DirectLightReservoirCommonParameters& parameters,
	    const FrameContext& frame,
	    const RenderView& view,
	    const PassRuntimeContext& passRuntimeContext)
	{
		parameters.Frame = passRuntimeContext.Frame;
		parameters.View = view.uniform;
		parameters.ViewCamera = view.cameraUniform;
		parameters.ViewLighting = frame.sceneGpuData->Lighting.Constants;
	}
}
