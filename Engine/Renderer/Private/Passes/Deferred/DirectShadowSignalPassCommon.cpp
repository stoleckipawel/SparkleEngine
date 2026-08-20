#include "../../PCH.h"

#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Passes/Deferred/DirectShadowSignalPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "View/RenderView.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"

void DirectShadowSignalPassCommon::SetParameters(
    DirectShadowSignalCommonPassParameters& parameters,
    const FrameContext& frame,
    const RenderView& view,
    const PassRuntimeContext& passRuntimeContext)
{
	parameters.Frame = passRuntimeContext.Frame;
	parameters.View = view.uniform;
	parameters.ViewCamera = view.cameraUniform;
	parameters.ViewTemporal = view.temporalUniform;
	parameters.SceneLighting = frame.preparedScene.gpuBindings->Lighting.Uniform;
}

void DirectShadowSignalPassCommon::SetRayQueryParameters(
    DirectShadowSignalRayQueryPassParameters& parameters,
    const FrameContext& frame,
    const RenderView& view,
    const PassRuntimeContext& passRuntimeContext,
    bool hasTraceableInstances)
{
	SetParameters(parameters, frame, view, passRuntimeContext);
	parameters.MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};

	parameters.MaterialTextureTable = frame.preparedScene.materialTextureTable.Binding;
	parameters.RayTracedShadows = RayTracedShadowPassData::Build(
	    passRuntimeContext.RayTracing,
	    hasTraceableInstances,
	    frame.preparedScene.gpuBindings->RayTracing.InstanceCount,
	    frame.preparedScene.gpuBindings->RayTracing.MaterialCount);
}
