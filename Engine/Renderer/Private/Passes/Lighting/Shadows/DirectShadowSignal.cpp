#include "../../../PCH.h"
#include "Passes/Lighting/Shadows/DirectShadowSignal.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Shadows/DirectShadowSignalShader.h"
#include "Passes/Lighting/Shadows/ShadowVisibility.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingMaterialPass.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "ShaderData/SceneShaderParameters.h"

template <typename TShader> static auto& BuildDirectShadowSignalParameters(
    FrameGraphBuilder& builder,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals)
{
	auto& parameters = builder.AllocParameters<TShader>();
	parameters->ShadowVisibilitySignal = builder.CreateUAV(shadowSignals.Visibility);
	parameters->CurrentReservoirSample = builder.CreateSRV(shadowSignals.ReservoirHistory.Sample.Current);
	parameters->CurrentReservoirWeight = builder.CreateSRV(shadowSignals.ReservoirHistory.Weight.Current);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	parameters->GBufferNormal = builder.CreateSRV(resources.Transient.GBuffer.Normal);
	BindSceneShaderParameters(builder, parameters, resources);
	BindRayTracedShadowParameters(builder, parameters);
	return parameters;
}

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    const DirectShadowSignalResources& shadowSignals,
    RenderRayTracingScene& rayTracingScene)
{
	AddRayTracingMaterialPass<DirectShadowSignalCS, DirectShadowSignalRGS>(
	    builder,
	    "DirectShadowSignal",
	    rayTracingScene,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u},
	    RayTracingDispatchDimensions{.Width = sceneExtent.Width, .Height = sceneExtent.Height, .Depth = 1u},
	    [&]<typename TShader>() -> auto& { return BuildDirectShadowSignalParameters<TShader>(builder, resources, shadowSignals); });
}
