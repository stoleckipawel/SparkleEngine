#include "../../PCH.h"
#include "Frame/Graph/ExecuteRenderFrameGraph.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Debug/RendererCVars.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Frame/Graph/RenderFrameGraphResourceBindings.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Frame/RenderFrameIdentity.h"
#include "Frame/RenderFrameTime.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "Passes/Presentation/ToneMappingSettings.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowPassInput.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RenderRayTracingFrameBindings.h"
#include "ShaderData/FrameUniformData.h"
#include "Textures/RendererTexture.h"
#include "View/RenderView.h"

static const auto g_renderFrameGraphExecutionLogger = Logging::GetOrCreateLogger("Renderer.FrameGraphExecution");

namespace RenderFrameGraphExecution
{
	FrameUniformData BuildFrameUniformData(std::uint64_t frameId, const RenderFrameTime& time) noexcept
	{
		return FrameUniformData{
		    .FrameIndex = static_cast<std::uint32_t>(frameId),
		    .TotalTimeSeconds = static_cast<float>(time.UnscaledTime.count()),
		    .DeltaTimeSeconds = static_cast<float>(time.UnscaledDelta.count()),
		    .ScaledTotalTimeSeconds = static_cast<float>(time.ScaledTime.count()),
		    .ScaledDeltaTimeSeconds = static_cast<float>(time.ScaledDelta.count())};
	}

	void BindRayTracingScene(
	    FrameGraph& frameGraph,
	    const RenderFrameGraphResources& resources,
	    const PreparedRenderScene& scene,
	    const RenderRayTracingFrameBindings& bindings)
	{
		if (!resources.SceneTlas.IsValid() || !bindings.HasBoundTlas())
		{
			Diagnostics::Fatal(
			    g_renderFrameGraphExecutionLogger,
			    __FILE__,
			    __LINE__,
			    "Frame-graph SceneTlas binding is incomplete.");
		}

		const RenderSceneGpuBindings& gpuBindings = *scene.gpuBindings;
		if (bindings.HasTraceableInstances()
		    && (gpuBindings.RayTracing.InstanceCount == 0u || gpuBindings.RayTracing.MaterialCount == 0u))
		{
			Diagnostics::Fatal(
			    g_renderFrameGraphExecutionLogger,
			    __FILE__,
			    __LINE__,
			    "Traceable SceneTlas instances have no matching hit-instance or material records.");
		}

		frameGraph.BindPersistentAccelerationStructure(resources.SceneTlas, bindings.TlasResource);
	}

	void BindSkyTexture(
	    FrameGraph& frameGraph,
	    const RenderFrameGraphResources& resources,
	    const PreparedRenderScene& scene)
	{
		const RendererTexture& skyTexture = *scene.sky.texture;
		frameGraph.BindPersistentTexture(
		    resources.ImportedScene.Sky,
		    skyTexture.Resource,
		    skyTexture.ShaderResourceView,
		    FrameGraphTextureDesc::CreateColor("Sky", skyTexture.Width, skyTexture.Height, skyTexture.Format),
		    ResourceState::ShaderResource);
	}

	void Execute(
	    FrameGraph& frameGraph,
	    const RenderFrameGraphResources& resources,
	    const RenderFrameIdentity& identity,
	    const RenderFrameTime& time,
	    const PreparedRenderScene& scene,
	    const RenderView& view,
	    const RenderRayTracingFrameBindings& rayTracingBindings,
	    RenderDeviceServices& deviceServices,
	    FrameExecutionDiagnostics& diagnostics,
	    TaskExecutor& taskExecutor)
	{
		BindRayTracingScene(frameGraph, resources, scene, rayTracingBindings);
		BindSkyTexture(frameGraph, resources, scene);
		BindRenderSceneGpuResources(frameGraph, resources.ImportedScene.Scene, *scene.gpuBindings);

		const RayTracedShadowSettings shadowSettings{
		    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
		    .MaxDistance = CVarRayTracedShadowMaxDistance.Get()};
		const RenderSceneGpuRayTracingBindings& rayTracing = scene.gpuBindings->RayTracing;

		frameGraph.ApplyPassParameterDefaults();
		frameGraph.ApplyParameters(BuildFrameUniformData(identity.FrameId, time));
		frameGraph.ApplyParameters(scene);
		frameGraph.ApplyParameters(view);
		frameGraph.ApplyParameters(
		    BuildExposureUniformData(view.displaySettings, static_cast<float>(time.UnscaledDelta.count())));
		frameGraph.ApplyParameters(BuildToneMappingUniformData(view.displaySettings.ToneMapper));
		frameGraph.ApplyParameters(
		    RayTracedShadowPassInput{
		        .Settings = shadowSettings,
		        .HitInstanceCount = rayTracing.InstanceCount,
		        .HitMaterialCount = rayTracing.MaterialCount});
		frameGraph.Setup();
		const FrameGraphPlan& plan = frameGraph.Compile();
		frameGraph.ApplyResourceProductionSetups();
		frameGraph.Execute(plan, deviceServices, diagnostics, taskExecutor);
	}
}
