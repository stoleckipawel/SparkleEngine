#pragma once

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"

template <typename TParameterInstance> void BindRayTracedShadowParameters(FrameGraphBuilder& builder, TParameterInstance& parameters)
{
	builder.AddParameterSetup<PreparedRenderScene>(
	    parameters,
	    [](auto& parameters, const PreparedRenderScene& scene)
	    {
		    const auto& rayTracing = scene.gpuBindings->RayTracing;
		    const bool hasInstances = rayTracing.InstanceCount != 0u;
		    parameters->RayTracedDirectionalShadowsEnabled = hasInstances ? 1u : 0u;
		    parameters->RayTracedLocalLightShadowsEnabled = hasInstances ? 1u : 0u;
		    parameters->RayTracedShadowNormalBias = hasInstances ? CVarRayTracedShadowNormalBias.Get() : 0.0f;
		    parameters->RayTracedShadowMaxDistance = hasInstances ? CVarRayTracedShadowMaxDistance.Get() : 0.0f;
	    });
}
