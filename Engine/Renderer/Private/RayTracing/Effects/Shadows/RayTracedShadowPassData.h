#pragma once

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Scene/Preparation/PreparedRenderScene.h"

RayTracedShadowUniformData BuildRayTracedShadowUniformData(const PreparedRenderScene& scene) noexcept;

template <typename TParameterInstance> void BindRayTracedShadowParameters(FrameGraphBuilder& builder, TParameterInstance& parameters)
{
	builder.AddParameterSetup<PreparedRenderScene>(
	    parameters,
	    [](auto& fields, const PreparedRenderScene& scene) { fields.RayTracedShadowConstants = BuildRayTracedShadowUniformData(scene); });
}
