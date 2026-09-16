#include "PCH.h"

#include "Passes/GBuffer/RayTracingGBuffer.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RayTracingGBufferShaders.h"
#include "RayTracing/RayTracingMaterialPass.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "ShaderData/SceneShaderParameters.h"

template <typename TShader>
static auto& BuildRayTracingGBufferParameters(FrameGraphBuilder& builder, const RenderFrameGraphResources& resources)
{
	const GBufferRenderTargets& targets = resources.Transient.GBuffer;
	auto& parameters = builder.AllocParameters<TShader>();
	parameters->GBufferBaseColor = builder.CreateUAV(targets.BaseColor);
	parameters->GBufferNormal = builder.CreateUAV(targets.Normal);
	parameters->GBufferMaterial = builder.CreateUAV(targets.Material);
	parameters->GBufferEmissive = builder.CreateUAV(targets.Emissive);
	parameters->GBufferSubsurface = builder.CreateUAV(targets.Subsurface);
	parameters->GBufferDeviceZ = builder.CreateUAV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
	BindSceneShaderParameters(builder, parameters, resources);
	return parameters;
}

void AddRayTracingGBufferMeshPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources,
    RenderRayTracingScene& rayTracingScene)
{
	AddRayTracingMaterialPass<RayTracingGBufferInlineCS, RayTracingGBufferRGS>(
	    builder,
	    "RayTracingGBuffer",
	    rayTracingScene,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u},
	    RayTracingDispatchDimensions{.Width = sceneExtent.Width, .Height = sceneExtent.Height, .Depth = 1u},
	    [&]<typename TShader>() -> auto& { return BuildRayTracingGBufferParameters<TShader>(builder, resources); });
}
