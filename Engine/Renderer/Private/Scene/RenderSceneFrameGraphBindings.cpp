#include "PCH.h"
#include "Scene/RenderSceneFrameGraphBindings.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/GpuScene/RenderSceneFrameGraphResources.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "Textures/RendererTexture.h"

static const auto g_renderSceneFrameGraphLogger = Logging::GetOrCreateLogger("Renderer.RenderSceneFrameGraph");

static void BindRayTracingScene(
    FrameGraph& frameGraph,
    const RenderFrameGraphResources& resources,
    const PreparedRenderScene& scene,
    const RenderRayTracingFrameBindings& bindings)
{
	if (!resources.SceneTlas.IsValid() || !bindings.HasBoundTlas())
	{
		Diagnostics::Fatal(g_renderSceneFrameGraphLogger, __FILE__, __LINE__, "Frame-graph SceneTlas binding is incomplete.");
	}

	const RenderSceneGpuBindings& gpuBindings = *scene.gpuBindings;
	if (bindings.HasTraceableInstances() && (gpuBindings.RayTracing.InstanceCount == 0u || gpuBindings.RayTracing.MaterialCount == 0u))
	{
		Diagnostics::Fatal(
		    g_renderSceneFrameGraphLogger,
		    __FILE__,
		    __LINE__,
		    "Traceable SceneTlas instances have no matching hit-instance or material records.");
	}

	frameGraph.BindPersistentAccelerationStructure(resources.SceneTlas, bindings.TlasResource);
}

static void BindSkyTexture(FrameGraph& frameGraph, const RenderFrameGraphResources& resources, const PreparedRenderScene& scene)
{
	const RendererTexture& skyTexture = *scene.sky.texture;

	frameGraph.BindPersistentTexture(
	    resources.ImportedScene.Sky,
	    skyTexture.Resource,
	    skyTexture.ShaderResourceView,
	    FrameGraphTextureDesc::CreateColor("Sky", skyTexture.Width, skyTexture.Height, skyTexture.Format),
	    ResourceState::ShaderResource);
}

void BindRenderSceneFrameGraphResources(
    FrameGraph& frameGraph,
    const RenderFrameGraphResources& resources,
    const PreparedRenderScene& scene,
    const RenderRayTracingFrameBindings& rayTracingBindings)
{
	BindRayTracingScene(frameGraph, resources, scene, rayTracingBindings);
	BindSkyTexture(frameGraph, resources, scene);
	BindRenderSceneGpuResources(frameGraph, resources.ImportedScene.Scene, *scene.gpuBindings);
}
