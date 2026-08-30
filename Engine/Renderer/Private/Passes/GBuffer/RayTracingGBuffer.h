#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
class RayTracingShaderTablePlan;
struct RayTracingCapabilityReport;
struct RenderFrameGraphImportedSceneResources;

void AddRayTracingGBufferMeshPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const RenderFrameGraphImportedSceneResources& externalResources,
    RayTracingShaderTablePlan& shaderTablePlan,
    const RayTracingCapabilityReport& capabilities);
