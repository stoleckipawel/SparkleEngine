#include "../../../PCH.h"
#include "Passes/Lighting/Shadows/DirectShadowSignal.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Shadows/DirectShadowSignalShader.h"
#include "Passes/Lighting/Shadows/DirectShadowSignalResources.h"
#include "Passes/Lighting/Shadows/DirectShadowControls.h"
#include "Passes/Lighting/LightingTargetClear.h"
#include <array>
#include "RayTracing/Effects/Shadows/RayTracedShadowPassData.h"
#include "RayTracing/RayTracingMaterialPass.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "ShaderData/SceneShaderParameters.h"

template <typename TShader>
static auto& BuildDirectShadowSignalParameters(FrameGraphBuilder& builder, const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<TShader>();
	parameters->ShadowVisibilitySignal = builder.CreateUAV(resources.Transient.ShadowVisibilitySignal);
	parameters->CurrentReservoirSample = builder.CreateSRV(resources.History.DirectLightReservoir.Sample.Current);
	parameters->CurrentReservoirWeight = builder.CreateSRV(resources.History.DirectLightReservoir.Weight.Current);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	parameters->GBufferWorldNormal = builder.CreateSRV(resources.Transient.GBuffer.WorldNormal);

	BindSceneShaderParameters(builder, parameters, resources);
	BindRayTracedShadowParameters(builder, parameters);

	return parameters;
}

void AddDirectShadowSignalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources,
    RenderRayTracingScene& rayTracingScene)
{
	CreateDirectShadowSignalResources(builder, sceneExtent, resources);
	if (!IsDirectShadowsActive())
	{
		const std::array targets{resources.Transient.ShadowVisibilitySignal};
		AddLightingTargetClearPass(builder, "DirectShadowSignalClear", sceneExtent, targets);
		return;
	}
	AddRayTracingMaterialPass<DirectShadowSignalCS, DirectShadowSignalRGS>(
	    builder,
	    "DirectShadowSignal",
	    rayTracingScene,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u},
	    RayTracingDispatchDimensions{.Width = sceneExtent.Width, .Height = sceneExtent.Height, .Depth = 1u},
	    [&]<typename TShader>() -> auto& { return BuildDirectShadowSignalParameters<TShader>(builder, resources); });
}
