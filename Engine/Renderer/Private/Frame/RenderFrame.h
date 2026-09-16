#pragma once

#include "Frame/RenderFrameIdentity.h"
#include "Frame/RenderFrameTime.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "View/RenderView.h"

#include <cstdint>

struct RenderFrame final
{
	RenderFrameIdentity Identity = {};
	RenderFrameTime Time = {};
	std::uint32_t FrameInFlightIndex = 0u;
	PreparedRenderScene PreparedScene = {};
	RenderView View = {};
	RenderRayTracingFrameBindings RayTracingBindings = {};
};
