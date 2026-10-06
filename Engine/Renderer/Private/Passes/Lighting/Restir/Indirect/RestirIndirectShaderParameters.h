#pragma once

#include "Passes/Lighting/Restir/Indirect/IndirectLightingControls.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingCVars.h"
#include "Frame/Graph/RenderFrameGraphResources.h"

template <typename TParameters> void BindRestirIndirectParameters(TParameters& parameters, const RenderFrameGraphResources& resources)
{
	const auto& gbuffer = resources.Transient.GBuffer;
	const auto& lighting = resources.Transient.Lighting;
	parameters->RestirIndirectBounceCount = ResolveRestirIndirectBounceCount();
	parameters->RestirIndirectTemporalReuse = CVarRestirIndirectTemporalReuse.Get() ? 1u : 0u;
	parameters->RestirIndirectSpatialReuse = CVarRestirIndirectSpatialReuse.Get() ? 1u : 0u;
	parameters->RestirIndirectEvaluateDiffuse = IsIndirectDiffuseActive(gbuffer.BaseColor, lighting.IndirectDiffuse) ? 1u : 0u;
	parameters->RestirIndirectEvaluateSpecular = IsIndirectSpecularActive(gbuffer.Material, lighting.IndirectSpecular) ? 1u : 0u;
	parameters->RestirIndirectTraceSecondaryShadows = IsIndirectShadowsActive() ? 1u : 0u;
}
