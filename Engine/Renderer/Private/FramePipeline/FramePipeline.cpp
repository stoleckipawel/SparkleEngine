#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Camera/RenderCamera.h"
#include "Commands/RenderCommandContext.h"
#include "Frame/RhiFrameConstants.h"
#include "Debug/RendererCVars.h"
#include "Diagnostics/FrameExecutionDiagnostics.h"
#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Builders/TemporalDataBuilder.h"
#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "Frame/Lighting/ReferenceLightingState.h"
#include "Frame/Lighting/RestirLightingState.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferDesc.h"
#include "Host/RendererSystemRoot.h"
#include "Pipeline/PipelineStateManager.h"
#include "Providers/RendererImageProviderStack.h"
#include "Providers/ImageProviderFrameContext.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include "RayTracing/Scene/RenderRayTracingPassServices.h"
#include "RayTracing/Scene/RenderRayTracingScene.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/GameScene.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "SceneData/Lifecycle/SceneRenderStateCoordinator.h"
#include "Textures/TextureManager.h"
#include "Time/Timer.h"
#include "Window/Window.h"

FramePipeline::FramePipeline(RendererSystemRoot& systems) noexcept : m_systems(&systems)
{
	m_frameExecutionDiagnostics.resize(RhiFrameConstants::FramesInFlight);
	m_frameContexts.resize(RhiFrameConstants::FramesInFlight);
	RenderDiagnostics& backendDiagnostics = m_systems->GetRenderHardwareInterface().GetDiagnostics();
	for (std::unique_ptr<FrameExecutionDiagnostics>& frameDiagnostics : m_frameExecutionDiagnostics)
	{
		frameDiagnostics = std::make_unique<FrameExecutionDiagnostics>(backendDiagnostics);
	}

	InitializeFrameGraph();
	CreateExposureHistoryResources();
	CreateDirectLightReservoirHistoryResources();
	CreateReferenceLightingHistoryResources();
	CreateRestirIndirectReservoirHistoryResources();
	BindWindowResizeEvent();
}

FramePipeline::~FramePipeline() noexcept
{
	ReleaseDirectLightReservoirHistoryResources();
	ReleaseReferenceLightingHistoryResources();
	ReleaseRestirIndirectReservoirHistoryResources();
	ReleaseExposureHistoryResources();
}

void FramePipeline::PrepareHostFrame() noexcept
{
	BeginFrame();
	SetupFrame();
}

void FramePipeline::RecordHostFrame() noexcept
{
	RecordFrame();
}

void FramePipeline::SubmitHostFrame() noexcept
{
	SubmitFrame();
	EndFrame();
}

void FramePipeline::OnRender() noexcept
{
	PrepareHostFrame();
	RecordHostFrame();
	SubmitHostFrame();
}

RenderViewportExtent FramePipeline::ResolveOutputExtent() const noexcept
{
	const Window& window = m_systems->GetWindow();
	if (m_viewportRenderRequest.Extent.IsValid())
	{
		return m_viewportRenderRequest.Extent;
	}

	return RenderViewportExtent{static_cast<std::uint32_t>(window.GetWidth()), static_cast<std::uint32_t>(window.GetHeight())};
}

FrameResolutionExtents FramePipeline::ResolveFrameResolution() const noexcept
{
	const RenderViewportExtent outputExtent = ResolveOutputExtent();
	const ImageProviderPipeline imagePipeline = GetLightingMode() == LightingMode::RestirPathTraced
	                                                ? ImageProviderPipeline::RayReconstruction
	                                                : ImageProviderPipeline::PresentationUpscaling;
	return FrameResolutionExtents{
	    .Render = m_systems->GetImageProviders().ResolveRenderExtent(outputExtent, imagePipeline),
	    .Output = outputExtent};
}

bool FramePipeline::ShouldOutputToBackBuffer() const noexcept
{
	return m_viewportRenderRequest.ViewportId == 0;
}

void FramePipeline::InitializeFrameGraph() noexcept
{
	InitializeFrameGraph(ResolveFrameResolution());
}

void FramePipeline::InitializeFrameGraph(FrameResolutionExtents resolution) noexcept
{
	const FrameGraphDependencies dependencies{
	    m_systems->GetRenderHardwareInterface(),
	    m_systems->GetWindow(),
	    resolution.Render,
	    resolution.Output,
	    ShouldOutputToBackBuffer()};

	FrameGraphFactory frameGraphFactory(dependencies);
	FrameGraphBuildResult buildResult = frameGraphFactory.Build();
	m_frameGraphRenderExtent = dependencies.renderExtent;
	m_frameGraphOutputExtent = dependencies.outputExtent;
	m_frameGraphPresentsToBackBuffer = dependencies.presentSceneToBackBuffer;
	m_frameResources = buildResult.Resources;
	m_gBufferMode = CVarGBufferMode.Get();
	m_lightingMode = GetLightingMode();
	m_referenceLightingSettingsKey = BuildReferenceLightingSettingsKey();
	m_restirLightingSettingsKey = BuildRestirLightingSettingsKey();
	m_imageProviderFrameGraphKey = m_systems->GetImageProviders().GetFrameGraphKey();
	m_frameGraph = std::move(buildResult.Graph);
}

void FramePipeline::BindWindowResizeEvent() noexcept
{
	auto handle = m_systems->GetWindow().OnResized.Add(
	    [this]()
	    {
		    m_bResizePending = true;
	    });
	m_resizeHandle = ScopedEventHandle(m_systems->GetWindow().OnResized, handle);
}

void FramePipeline::RefreshFrameExecution() noexcept
{
	RefreshFrameExecution(ResolveFrameResolution());
}

void FramePipeline::RefreshFrameExecution(FrameResolutionExtents resolution) noexcept
{
	RenderDeviceServices& backend = m_systems->GetBackend();
	backend.Flush();
	for (std::unique_ptr<FrameContext>& frameContext : m_frameContexts)
	{
		frameContext.reset();
	}

	m_frameGraph.reset();
	InitializeFrameGraph(resolution);
	CreateDirectLightReservoirHistoryResources();
	CreateReferenceLightingHistoryResources();
	CreateRestirIndirectReservoirHistoryResources();
}

void FramePipeline::BeginFrame() noexcept
{
	RenderDeviceServices& backend = m_systems->GetBackend();
	TemporalDataBuilder& temporalDataBuilder = m_systems->GetTemporalDataBuilder();

	if (m_bResizePending)
	{
		m_bResizePending = false;
		temporalDataBuilder.ResetHistory("Window resize");
		ResetExposureHistory();
		ResetReferenceLightingHistory();
		ResetRestirLightingHistory();
		m_systems->GetImageProviders().ResetHistory();

		if (m_systems->GetWindow().HasValidSize())
		{
			backend.Flush();
			backend.ResizeSwapChain();
			RefreshFrameExecution(ResolveFrameResolution());
		}
	}

	const FrameResolutionExtents frameResolution = ResolveFrameResolution();
	const bool presentsToBackBuffer = ShouldOutputToBackBuffer();
	const bool resolutionChanged = frameResolution.Render.Width != m_frameGraphRenderExtent.Width ||
	                               frameResolution.Render.Height != m_frameGraphRenderExtent.Height ||
	                               frameResolution.Output.Width != m_frameGraphOutputExtent.Width ||
	                               frameResolution.Output.Height != m_frameGraphOutputExtent.Height;
	const bool presentationChanged = presentsToBackBuffer != m_frameGraphPresentsToBackBuffer;
	if (resolutionChanged || presentationChanged)
	{
		temporalDataBuilder.ResetHistory(presentationChanged ? "Frame presentation mode changed" : "Frame resolution changed");
		ResetExposureHistory();
		ResetReferenceLightingHistory();
		ResetRestirLightingHistory();
		m_systems->GetImageProviders().ResetHistory();
		RefreshFrameExecution(frameResolution);
	}

	const GBufferMode gBufferMode = CVarGBufferMode.Get();
	if (gBufferMode != m_gBufferMode)
	{
		temporalDataBuilder.ResetHistory("GBuffer mode changed");
		ResetExposureHistory();
		ResetReferenceLightingHistory();
		ResetRestirLightingHistory();
		m_systems->GetImageProviders().ResetHistory();
		RefreshFrameExecution(ResolveFrameResolution());
	}

	const LightingMode lightingMode = GetLightingMode();
	if (lightingMode != m_lightingMode)
	{
		temporalDataBuilder.ResetHistory("Lighting mode changed");
		ResetExposureHistory();
		ResetReferenceLightingHistory();
		ResetRestirLightingHistory();
		m_systems->GetImageProviders().ResetHistory();
		RefreshFrameExecution(ResolveFrameResolution());
	}

	const std::uint64_t referenceLightingSettingsKey = BuildReferenceLightingSettingsKey();
	if (referenceLightingSettingsKey != m_referenceLightingSettingsKey)
	{
		m_referenceLightingSettingsKey = referenceLightingSettingsKey;
		if (lightingMode == LightingMode::ReferencePathTraced)
		{
			ResetReferenceLightingHistory();
			m_systems->GetImageProviders().ResetHistory();
		}
	}

	const std::uint64_t restirLightingSettingsKey = BuildRestirLightingSettingsKey();
	if (restirLightingSettingsKey != m_restirLightingSettingsKey)
	{
		m_restirLightingSettingsKey = restirLightingSettingsKey;
		if (lightingMode == LightingMode::RestirPathTraced)
		{
			ResetRestirLightingHistory();
			m_systems->GetImageProviders().ResetHistory();
		}
	}

	const ImageProviderGraphKey imageProviderFrameGraphKey = m_systems->GetImageProviders().GetFrameGraphKey();
	if (imageProviderFrameGraphKey != m_imageProviderFrameGraphKey)
	{
		temporalDataBuilder.ResetHistory("Image provider graph mode changed");
		ResetExposureHistory();
		ResetReferenceLightingHistory();
		ResetRestirLightingHistory();
		m_systems->RefreshImageProviders();
		RefreshFrameExecution(ResolveFrameResolution());
		m_systems->GetImageProviders().ResetHistory();
		m_imageProviderFrameGraphKey = imageProviderFrameGraphKey;
	}

	backend.BeginFrame();
	m_frameContexts[m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex()].reset();
	m_systems->TickDiagnostics(m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex());
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();
	frameDiagnostics.ResolveTimings();
}

void FramePipeline::SetupFrame() noexcept
{
	Timer& timer = m_systems->GetTimer();
	timer.Tick();
	RefreshViewportRenderProducts();

	m_sceneSnapshot.Capture(m_systems->GetGameScene().CaptureSnapshot());
	m_systems->GetTextureManager().LoadSceneTextures(m_sceneSnapshot.textures);
	m_systems->GetRenderCamera().Update(m_sceneSnapshot.camera);

	const RenderViewportExtent renderExtent =
	    m_frameGraphRenderExtent.IsValid() ? m_frameGraphRenderExtent : ResolveFrameResolution().Render;
	m_perFrameData = m_perFrameDataBuilder.Build(timer, CVarRenderViewMode.Get(), renderExtent);
}

void FramePipeline::RefreshViewportRenderProducts() noexcept
{
	const FrameResolutionExtents resolution =
	    m_frameGraphOutputExtent.IsValid() && m_frameGraphRenderExtent.IsValid()
	        ? FrameResolutionExtents{.Render = m_frameGraphRenderExtent, .Output = m_frameGraphOutputExtent}
	        : ResolveFrameResolution();

	m_viewportRenderProducts.Clear();
	m_viewportRenderProducts.SetProduct(
	    RenderOutputFlags::SceneColor,
	    RenderProduct{
	        ToRenderProductHandle(m_frameResources.ViewportProducts.FinalSceneColor),
	        resolution.Output,
	        RenderProductFormat::ColorLdr});

	if (m_frameResources.ViewportProducts.SceneDepth.IsValid() &&
	    HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::SceneDepth,
		    RenderProduct{
		        ToRenderProductHandle(m_frameResources.ViewportProducts.SceneDepth),
		        resolution.Render,
		        RenderProductFormat::Float});
	}

	if (m_frameResources.ViewportProducts.Normals.IsValid() &&
	    HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::Normals))
	{
		m_viewportRenderProducts.SetProduct(
		    RenderOutputFlags::Normals,
		    RenderProduct{
		        ToRenderProductHandle(m_frameResources.ViewportProducts.Normals),
		        resolution.Render,
		        RenderProductFormat::ColorHdr});
	}
}

void FramePipeline::RecordFrame() noexcept
{
	RenderHardwareInterface& renderHardwareInterface = m_systems->GetRenderHardwareInterface();
	const RayTracedShadowSettings shadowSettings{
	    .NormalBias = CVarRayTracedShadowNormalBias.Get(),
	    .MaxDistance = CVarRayTracedShadowMaxDistance.Get()};
	const bool rayTracedShadowsEnabled = CVarRayTracedShadowsEnabled.Get();
	RenderRayTracingScene* activeRayTracingScene = m_systems->GetRenderRayTracingScene();
	std::string temporalResetReason;
	SceneRenderStateCoordinator* sceneRenderStateCoordinator = m_systems->GetSceneRenderStateCoordinator();
	if (sceneRenderStateCoordinator != nullptr && sceneRenderStateCoordinator->ConsumeTemporalHistoryResetRequest(temporalResetReason))
	{
		m_systems->GetTemporalDataBuilder().ResetHistory(temporalResetReason);
		ResetExposureHistory();
		ResetReferenceLightingHistory();
		ResetRestirLightingHistory();
	}

	std::unique_ptr<FrameContext>& frameSlot = m_frameContexts[renderHardwareInterface.GetCurrentFrameIndex()];
	frameSlot = [&]()
	{
		return std::make_unique<FrameContext>(BuildFrameContext(
		    m_sceneSnapshot,
		    renderHardwareInterface,
		    m_systems->GetRenderCamera(),
		    m_frameGraphRenderExtent,
		    m_systems->GetRenderSceneDataBuilder(),
		    activeRayTracingScene,
		    m_systems->GetPerViewDataBuilder(),
		    m_systems->GetTemporalDataBuilder()));
	}();
	FrameContext& frame = *frameSlot;
	UpdateLightingHistoryState(frame);
	if (frame.mainView.perTemporalData.HistoryValid == 0u)
	{
		ResetExposureHistory();
		ResetReferenceLightingHistory();
		ResetRestirLightingHistory();
	}

	m_systems->GetImageProviders().SetupFrame(
	    ImageProviderFrameContext{
	        .RenderExtent = m_frameGraphRenderExtent,
	        .OutputExtent = m_frameGraphOutputExtent,
	        .FrameIndex = m_systems->GetTimer().GetFrameCount(),
	        .Camera = frame.mainView.perViewData.Camera,
	        .TemporalData = frame.mainView.perTemporalData,
	        .TemporalState = frame.mainView.temporalState,
	        .ResetHistory = frame.mainView.perTemporalData.HistoryValid == 0u});

	if (m_frameGraph != nullptr && m_frameResources.SceneTlas.IsValid())
	{
		if (activeRayTracingScene != nullptr)
		{
			frame.rayTracingScene = activeRayTracingScene->Prepare(frame.sceneData);
			if (frame.rayTracingScene.HasBoundTlas())
			{
				m_frameGraph->BindPersistentAccelerationStructure(
				    m_frameResources.SceneTlas,
				    frame.rayTracingScene.TlasResource,
				    frame.rayTracingScene.TlasGpuAddress);
			}
			else
			{
				m_frameGraph->ClearPersistentAccelerationStructureBinding(m_frameResources.SceneTlas);
			}
		}
		else
		{
			m_frameGraph->ClearPersistentAccelerationStructureBinding(m_frameResources.SceneTlas);
		}
	}

	BindExposureHistoryFrameGraphResources();
	if (frame.sceneData.sky.skyTexture != nullptr)
	{
		m_frameGraph->BindPersistentTexture(m_frameResources.External.Sky, *frame.sceneData.sky.skyTexture, ResourceState::ShaderResource);
	}
	else
	{
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.External.Sky);
	}
	const auto bindFrameBuffer = [this](FrameGraphBufferHandle handle, const FrameBufferResource& buffer, const char* name)
	{
		if (buffer)
		{
			m_frameGraph->BindPersistentBuffer(
			    handle,
			    buffer.GetResource(),
			    FrameGraphBufferDesc::Create(name, buffer.GetSizeInBytes(), buffer.GetStrideInBytes()),
			    ResourceState::ShaderResource);
		}
		else
		{
			m_frameGraph->ClearPersistentBufferBinding(handle);
		}
	};
	bindFrameBuffer(m_frameResources.External.DirectionalLights, frame.lighting.GetDirectionalLightsBuffer(), "DirectionalLights");
	bindFrameBuffer(m_frameResources.External.PointLights, frame.lighting.GetPointLightsBuffer(), "PointLights");
	bindFrameBuffer(m_frameResources.External.SpotLights, frame.lighting.GetSpotLightsBuffer(), "SpotLights");
	bindFrameBuffer(m_frameResources.External.RectLights, frame.lighting.GetRectLightsBuffer(), "RectLights");
	bindFrameBuffer(m_frameResources.External.MeshInstances, frame.meshInstances.GetBuffer(), "MeshInstances");
	bindFrameBuffer(m_frameResources.External.RayTracingHitVertices, frame.rayTracingHitData.GetVertexBuffer(), "RayTracingHitVertices");
	bindFrameBuffer(
	    m_frameResources.External.RayTracingHitSkinInfluences,
	    frame.rayTracingHitData.GetSkinInfluenceBuffer(),
	    "RayTracingHitSkinInfluences");
	bindFrameBuffer(m_frameResources.External.RayTracingHitIndices, frame.rayTracingHitData.GetIndexBuffer(), "RayTracingHitIndices");
	bindFrameBuffer(
	    m_frameResources.External.RayTracingHitInstances,
	    frame.rayTracingHitData.GetInstanceBuffer(),
	    "RayTracingHitInstances");
	bindFrameBuffer(
	    m_frameResources.External.RayTracingHitMaterials,
	    frame.rayTracingHitData.GetMaterialBuffer(),
	    "RayTracingHitMaterials");
	bindFrameBuffer(m_frameResources.External.JointMatrices, frame.skinning.GetBuffer(), "JointMatrices");
	bindFrameBuffer(m_frameResources.External.PreviousJointMatrices, frame.skinning.GetPreviousBuffer(), "PreviousJointMatrices");
	BindDirectLightReservoirHistoryFrameGraphResources();
	BindReferenceLightingHistoryFrameGraphResources();
	BindRestirIndirectReservoirHistoryFrameGraphResources();

	m_frameGraph->Setup(frame);
	const FrameGraphPlan& compiledPlan = m_frameGraph->Compile();
	RenderCommandList& commandList = m_systems->GetBackend().GetCurrentGraphicsCommandList();
	RenderCommandContext cmd(commandList);
	const RenderRayTracingPassServices rayTracingPassServices{
	    .Scene = activeRayTracingScene,
	    .CapabilityReport = activeRayTracingScene != nullptr ? &activeRayTracingScene->GetCapabilities() : nullptr,
	    .ShadowSettings = &shadowSettings,
	    .ShadowsEnabled = rayTracedShadowsEnabled};
	const RendererImageProviderPassServices imageProviderPassServices = m_systems->GetImageProviders().BuildPassServices();
	const PassRuntimeServices passRuntimeServices{
	    .HardwareInterface = renderHardwareInterface,
	    .RuntimeManager = m_systems->GetPipelineStateManager(),
	    .PerFrame = m_perFrameData,
	    .ExposureHistoryValid = m_exposureHistoryValid,
	    .ReferenceLightingHistoryValid = m_referenceLightingHistoryValid,
	    .DirectLightReservoirHistoryValid = m_directLightReservoirHistoryValid,
	    .RestirIndirectReservoirHistoryValid = m_restirIndirectReservoirHistoryValid,
	    .Textures = &m_systems->GetTextureManager(),
	    .RayTracing = &rayTracingPassServices,
	    .ImageProviders = &imageProviderPassServices};
	FrameExecutionDiagnostics& frameDiagnostics = GetCurrentFrameDiagnostics();

	auto gpuFrameScope =
	    CVarRendererDiagnosticMarkerVerbosity.Get() != RendererDiagnosticMarkerVerbosity::Off
	        ? frameDiagnostics.BeginGpuScope(cmd, "GPU Frame", RhiDiagnosticLabelColor{.Red = 180, .Green = 200, .Blue = 220, .Alpha = 255})
	        : ScopedGpuScope{};

	m_frameGraph->Execute(compiledPlan, cmd, frame, passRuntimeServices, frameDiagnostics);
}

void FramePipeline::SubmitFrame() noexcept
{
	m_systems->GetBackend().SubmitFrame();
}

void FramePipeline::EndFrame() noexcept
{
	const bool restirLightingActive = GetLightingMode() == LightingMode::RestirPathTraced;
	m_exposureHistoryValid = HasExposureHistoryResources();
	m_referenceLightingHistoryValid = GetLightingMode() == LightingMode::ReferencePathTraced && HasReferenceLightingHistoryResources();
	m_directLightReservoirHistoryValid = restirLightingActive && HasDirectLightReservoirHistoryResources();
	m_restirIndirectReservoirHistoryValid = restirLightingActive && HasRestirIndirectReservoirHistoryResources();
	m_systems->GetBackend().AdvanceFrameInFlight();
}
