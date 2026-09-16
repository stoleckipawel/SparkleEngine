#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerResources.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerShader.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerUniformData.h"
#include "RayTracing/RayTracingExecutionFrontend.h"
#include "RayTracing/RayTracingMaterialPass.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"
#include "ShaderData/SceneShaderParameters.h"

template <typename TShader> static auto& BuildReferencePathTracerParameters(
    FrameGraphBuilder& builder,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData)
{
	auto& parameters = builder.AllocParameters<TShader>();
	parameters->WorkingMean = builder.CreateUAV(graphResources.WorkingMean);
	parameters->WorkingM2 = builder.CreateUAV(graphResources.WorkingM2);
	parameters->CommittedMean = builder.CreateSRV(graphResources.CommittedMean);
	parameters->CommittedM2 = builder.CreateSRV(graphResources.CommittedM2);
	parameters->SamplerLinearWrapClamp = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::None,
	    .Address =
	        RhiSamplerAddressModes{.U = RhiSamplerAddressMode::Wrap, .V = RhiSamplerAddressMode::Clamp, .W = RhiSamplerAddressMode::Clamp}};
	BindSceneShaderParameters(builder, parameters, resources);
	builder.AddPassParameterSetup(
	    parameters,
	    [uniformData = &uniformData](auto& fields) { fields.ReferencePathTracerConstants = *uniformData; });
	return parameters;
}

static void AddReferencePathTracerDisplayPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData)
{
	auto& display = builder.AllocParameters<ReferencePathTracerDisplayCS>();
	display->WorkingMean = builder.CreateSRV(graphResources.WorkingMean);
	display->WorkingM2 = builder.CreateSRV(graphResources.WorkingM2);
	display->CommittedMean = builder.CreateUAV(graphResources.CommittedMean);
	display->CommittedM2 = builder.CreateUAV(graphResources.CommittedM2);
	display->SceneColor = builder.CreateUAV(resources.Transient.Scene.SceneColor);
	builder.AddPassParameterSetup(
	    display,
	    [uniformData = &uniformData](auto& fields) { fields.ReferencePathTracerConstants = *uniformData; });
	builder.Dispatch<ReferencePathTracerDisplayCS>(
	    "ReferencePathTracer.CommittedDisplay",
	    display,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(extent.Width, 8u), MathUtils::DivideRoundUp(extent.Height, 8u), 1u});
}

void AddReferencePathTracerGpuPasses(
    FrameGraphBuilder& builder,
    RenderViewportExtent extent,
    const RenderFrameGraphResources& resources,
    const ReferencePathTracerGraphResources& graphResources,
    const ReferencePathTracerUniformData& uniformData,
    std::uint32_t workRowsPerDispatch,
    RenderRayTracingScene& rayTracingScene)
{
	if (rayTracingScene.GetExecutionFrontend() != RayTracingExecutionFrontend::None)
	{
		AddRayTracingMaterialPass<ReferencePathTracerInlineCS, ReferencePathTracerRGS>(
		    builder,
		    "ReferencePathTracer.SurfaceTransportReference",
		    rayTracingScene,
		    ComputeDispatchDesc{MathUtils::DivideRoundUp(extent.Width, 8u), MathUtils::DivideRoundUp(workRowsPerDispatch, 8u), 1u},
		    RayTracingDispatchDimensions{.Width = extent.Width, .Height = workRowsPerDispatch, .Depth = 1u},
		    [&]<typename TShader>() -> auto&
		    { return BuildReferencePathTracerParameters<TShader>(builder, resources, graphResources, uniformData); });
	}
	AddReferencePathTracerDisplayPass(builder, extent, resources, graphResources, uniformData);
}
