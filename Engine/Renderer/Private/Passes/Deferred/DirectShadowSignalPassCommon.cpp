#include "../../PCH.h"
#include "Passes/Deferred/DirectShadowSignalPassCommon.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "Frame/Lighting/ShadowVisibility.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Bindings/LightingPassBinding.h"
#include "Passes/Bindings/RayTracedShadowPassBinding.h"

namespace
{
	template <typename TParameters> class ParameterReference final
	{
	  public:
		explicit ParameterReference(TParameters& parameters) noexcept : m_parameters(&parameters) {}

		TParameters* operator->() noexcept { return m_parameters; }

	  private:
		TParameters* m_parameters;
	};
}

void DirectShadowSignalPassCommon::DeclareResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneDepth,
    const DirectShadowSignalResources& shadowSignals,
    DirectShadowSignalCommonPassParameters& parameters)
{
	parameters.ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
	parameters.CurrentReservoirSample = builder.CreateSRV(shadowSignals.CurrentReservoirSample);
	parameters.CurrentReservoirWeight = builder.CreateSRV(shadowSignals.CurrentReservoirWeight);
	parameters.SceneDepth = builder.CreateSRV(sceneDepth);
}

void DirectShadowSignalPassCommon::DeclareRayQueryResources(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneDepth,
    const GBufferRenderTargets& gbuffer,
    const DirectShadowSignalResources& shadowSignals,
    DirectShadowSignalRayQueryPassParameters& parameters)
{
	DeclareResources(builder, sceneDepth, shadowSignals, parameters);
	parameters.GBufferNormal = builder.CreateSRV(gbuffer.Normal);
}

void DirectShadowSignalPassCommon::SetParameters(
    DirectShadowSignalCommonPassParameters& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices)
{
	parameters.PerFrame = passRuntimeServices.PerFrame;
	parameters.PerView = viewData.perViewData;
	ParameterReference parameterReference(parameters);
	LightingPassBinding::SetParameters(parameterReference, frame);
}

void DirectShadowSignalPassCommon::SetRayQueryParameters(
    DirectShadowSignalRayQueryPassParameters& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices,
    bool hasSceneTlas)
{
	SetParameters(parameters, frame, viewData, passRuntimeServices);
	ParameterReference parameterReference(parameters);
	RayTracedShadowPassBinding::SetRayQueryParameters(parameterReference, frame, passRuntimeServices, hasSceneTlas);
}
