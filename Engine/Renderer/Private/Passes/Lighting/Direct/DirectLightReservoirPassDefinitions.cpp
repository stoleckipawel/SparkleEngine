#include "../../../PCH.h"
#include "Passes/Lighting/Direct/DirectLightReservoirPassDefinitions.h"

#include "Core/Public/Math/MathUtils.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Direct/DirectLightReservoirSpatialShader.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"
#include "Passes/Lighting/Direct/DirectLightReservoirTemporalShader.h"
#include "ShaderData/SceneShaderParameters.h"

template <typename Parameters>
static void BindDirectLightReservoirSurface(FrameGraphBuilder& builder, Parameters& parameters, const RenderFrameGraphResources& resources)
{
	const GBufferRenderTargets& gbuffer = resources.Transient.GBuffer;
	parameters->GBufferBaseColor = builder.CreateSRV(gbuffer.BaseColor);
	parameters->GBufferWorldNormal = builder.CreateSRV(gbuffer.WorldNormal);
	parameters->GBufferMaterial = builder.CreateSRV(gbuffer.Material);
	parameters->GBufferSubsurface = builder.CreateSRV(gbuffer.Subsurface);
	parameters->SceneDepth = builder.CreateSRV(resources.Transient.Scene.SceneDepth);
	BindSceneShaderParameters(builder, parameters, resources);

	builder.AddPassParameterSetup(
	    parameters,
	    [baseColorInput = gbuffer.BaseColor,
	        materialInput = gbuffer.Material,
	        subsurfaceInput = gbuffer.Subsurface,
	        diffuseOutput = resources.Transient.Lighting.DirectDiffuse,
	        specularOutput = resources.Transient.Lighting.DirectSpecular,
	        subsurfaceOutput = resources.Transient.Lighting.DirectSubsurface](auto& fields)
	    {
		    fields.DirectLightingConstants =
		        BuildDirectLightingUniform(baseColorInput, materialInput, subsurfaceInput, diffuseOutput, specularOutput, subsurfaceOutput);
	    });
}

void AddDirectLightReservoirTemporalPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<DirectLightReservoirTemporalCS>();
	parameters->TemporalReservoirSample = builder.CreateUAV(resources.Transient.DirectLightTemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateUAV(resources.Transient.DirectLightTemporalReservoirWeight);
	parameters->PreviousReservoirSample = builder.CreateSRV(resources.History.DirectLightReservoir.Sample.Previous);
	parameters->PreviousReservoirWeight = builder.CreateSRV(resources.History.DirectLightReservoir.Weight.Previous);
	parameters->PreviousReservoirSurface = builder.CreateSRV(resources.History.DirectLightReservoir.Surface.Previous);
	parameters->GBufferMotionVector = builder.CreateSRV(resources.Transient.GBuffer.MotionVector);

	BindDirectLightReservoirSurface(builder, parameters, resources);

	const auto invalidateTemporalHistory = [](auto& fields, bool hasBeenProduced)
	{
		if (!hasBeenProduced)
		{
			ViewTemporalUniformData temporal = *fields.ViewTemporal.GetValue();
			temporal.HistoryValid = 0u;
			fields.ViewTemporal = temporal;
		}
	};

	builder.AddResourceProductionSetup(parameters, resources.History.DirectLightReservoir.Sample.Previous, invalidateTemporalHistory);
	builder.AddResourceProductionSetup(parameters, resources.History.DirectLightReservoir.Weight.Previous, invalidateTemporalHistory);
	builder.AddResourceProductionSetup(parameters, resources.History.DirectLightReservoir.Surface.Previous, invalidateTemporalHistory);

	builder.Dispatch<DirectLightReservoirTemporalCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}

void AddDirectLightReservoirSpatialPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const RenderFrameGraphResources& resources)
{
	auto& parameters = builder.AllocParameters<DirectLightReservoirSpatialCS>();
	parameters->TemporalReservoirSample = builder.CreateSRV(resources.Transient.DirectLightTemporalReservoirSample);
	parameters->TemporalReservoirWeight = builder.CreateSRV(resources.Transient.DirectLightTemporalReservoirWeight);
	parameters->CurrentReservoirSample = builder.CreateUAV(resources.History.DirectLightReservoir.Sample.Current);
	parameters->CurrentReservoirWeight = builder.CreateUAV(resources.History.DirectLightReservoir.Weight.Current);
	parameters->CurrentReservoirSurface = builder.CreateUAV(resources.History.DirectLightReservoir.Surface.Current);

	BindDirectLightReservoirSurface(builder, parameters, resources);

	builder.Dispatch<DirectLightReservoirSpatialCS>(
	    parameters,
	    ComputeDispatchDesc{MathUtils::DivideRoundUp(sceneExtent.Width, 8u), MathUtils::DivideRoundUp(sceneExtent.Height, 8u), 1u});
}
