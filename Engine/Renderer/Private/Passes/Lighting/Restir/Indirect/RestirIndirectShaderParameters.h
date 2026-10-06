#pragma once

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"
#include "Frame/Graph/RenderFrameGraphResources.h"

template <typename TParameters>
void BindRestirIndirectParameters(FrameGraphBuilder& builder, TParameters& parameters, const RenderFrameGraphResources& resources)
{
	const auto& gbuffer = resources.Transient.GBuffer;
	const auto& lighting = resources.Transient.Lighting;
	builder.AddPassParameterSetup(
	    parameters,
	    [baseColor = gbuffer.BaseColor,
	        material = gbuffer.Material,
	        diffuse = lighting.IndirectDiffuse,
	        specular = lighting.IndirectSpecular](auto& parameters)
	    {
		    parameters->RestirIndirectBounceCount = ResolveRestirIndirectBounceCount();
		    parameters->RestirIndirectTemporalReuse = CVarRestirIndirectTemporalReuse.Get() ? 1u : 0u;
		    parameters->RestirIndirectSpatialReuse = CVarRestirIndirectSpatialReuse.Get() ? 1u : 0u;
		    parameters->RestirIndirectEvaluateDiffuse = IsIndirectDiffuseActive(baseColor, diffuse) ? 1u : 0u;
		    parameters->RestirIndirectEvaluateSpecular = IsIndirectSpecularActive(material, specular) ? 1u : 0u;
		    parameters->RestirIndirectTraceSecondaryShadows = IsIndirectShadowsActive() ? 1u : 0u;
	    });
}
