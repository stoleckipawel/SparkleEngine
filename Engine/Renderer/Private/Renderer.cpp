#include "PCH.h"
#include "Renderer.h"

#include "Level/LevelManager.h"
#include "RHI/Public/Interop/Internal/RendererBackendServicesAccess.h"
#include "RHI/Public/Interop/RendererBackendServices.h"
#include "Window/Window.h"
#include "Textures/TextureManager.h"
#include "GPU/GPUMeshCache.h"
#include "Scene/GameScene.h"
#include "Time/Timer.h"
#include "Camera/RenderCamera.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "GPU/CommandContext.h"
#include "Frame/FrameContext.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/RenderPassContext.h"
#include "Scene/Camera/CameraComponent.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "Frame/Builders/BuildFrameContext.h"
#include "Frame/Builders/PerViewDataBuilder.h"
#include "Frame/Shadow/ShadowBuilder.h"
#include "Frame/Shadow/ShadowFrameBuilder.h"
#include "Frame/Builders/ViewLightingBuilder.h"
#include "Pipeline/PipelineStateManager.h"
#include "SceneData/Builders/RenderSceneDataBuilder.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "SceneData/Lifecycle/RenderSceneSnapshot.h"
#include "SceneData/Lifecycle/SceneRenderStateCoordinator.h"

Renderer::Renderer(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept :
    m_timer(&timer), m_gameScene(&gameScene), m_window(&window)
{
	InitializeCoreSystems();

	InitializeSceneSystems(levelManager);
	InitializeFrameGraph();
	BindWindowResizeEvent();

	PostLoad();
}

RenderHardwareInterface& Renderer::GetRenderHardwareInterface() noexcept
{
	return m_backend->GetRenderHardwareInterface();
}

const RenderHardwareInterface& Renderer::GetRenderHardwareInterface() const noexcept
{
	return m_backend->GetRenderHardwareInterface();
}

void Renderer::PrepareHostFrame() noexcept
{
	BeginFrame();
	SetupFrame();
}

void Renderer::RecordHostFrame() noexcept
{
	RecordFrame();
}

void Renderer::SubmitHostFrame() noexcept
{
	SubmitFrame();
	EndFrame();
}

std::uint64_t Renderer::ResolveRenderProductTextureId(RenderProductHandle handle) const noexcept
{
	if (!handle || !m_frameGraph)
	{
		return 0;
	}

	const ResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	return m_frameGraph->ResolveShaderResourceView(TextureHandle{resourceHandle}).Value;
}

void Renderer::TransitionRenderProduct(
    NativeGraphicsCommandListHandle commandList,
    RenderProductHandle handle,
    ResourceState before,
    ResourceState after) const noexcept
{
	if (!commandList || !handle || !m_frameGraph || before == after)
	{
		return;
	}

	auto* nativeCommandList = static_cast<ID3D12GraphicsCommandList*>(commandList.Value);
	if (nativeCommandList == nullptr)
	{
		return;
	}

	const ResourceHandle resourceHandle{static_cast<std::uint32_t>(handle.Value - 1ull)};
	const NativeResourceHandle resource = m_frameGraph->ResolveResource(TextureHandle{resourceHandle});
	if (!resource)
	{
		return;
	}

	CommandContext cmd(nativeCommandList);
	cmd.TransitionResource(resource, before, after);
}

void Renderer::InitializeCoreSystems() noexcept
{
	m_backend = RendererBackendServices::Create(*m_timer, *m_window);
	auto& rhi = Rhi::Internal::RendererBackendServicesAccess::GetRhi(*m_backend);
	m_pipelineStateManager = std::make_unique<PipelineStateManager>(rhi);
	m_gpuMeshCache = std::make_unique<GPUMeshCache>(rhi);
}

void Renderer::InitializeSceneSystems(LevelManager& levelManager) noexcept
{
	auto& rhi = Rhi::Internal::RendererBackendServicesAccess::GetRhi(*m_backend);
	auto& descriptorHeapManager = Rhi::Internal::RendererBackendServicesAccess::GetDescriptorHeapManager(*m_backend);
	m_textureManager = std::make_unique<TextureManager>(rhi, descriptorHeapManager);
	m_materialCacheManager = std::make_unique<MaterialCacheManager>(*m_textureManager, descriptorHeapManager);
	m_renderSceneDataBuilder = std::make_unique<RenderSceneDataBuilder>(*m_materialCacheManager, *m_gpuMeshCache);
	m_perViewDataBuilder = std::make_unique<PerViewDataBuilder>();
	m_viewLightingBuilder = std::make_unique<ViewLightingBuilder>();
	m_sceneSnapshot = std::make_unique<RenderSceneSnapshot>();
	m_shadowBuilder = std::make_unique<ShadowBuilder>();
	m_shadowFrameBuilder = std::make_unique<ShadowFrameBuilder>();

	m_renderCamera = std::make_unique<RenderCamera>();

	m_sceneRenderStateCoordinator = std::make_unique<SceneRenderStateCoordinator>(
	    levelManager.GetLevelChangeEvents(),
	    *m_gameScene,
	    rhi,
	    *m_gpuMeshCache,
	    *m_textureManager,
	    *m_sceneSnapshot,
	    *m_renderCamera,
	    *m_materialCacheManager);
}

RenderViewportExtent Renderer::ResolveSceneExtent() const noexcept
{
	if (m_viewportRenderRequest.Extent.IsValid())
	{
		return m_viewportRenderRequest.Extent;
	}

	return RenderViewportExtent{static_cast<std::uint32_t>(m_window->GetWidth()), static_cast<std::uint32_t>(m_window->GetHeight())};
}

bool Renderer::ShouldPresentSceneToBackBuffer() const noexcept
{
	return m_viewportRenderRequest.ViewportId == 0;
}

void Renderer::InitializeFrameGraph() noexcept
{
	auto& rhi = Rhi::Internal::RendererBackendServicesAccess::GetRhi(*m_backend);
	auto& swapChain = Rhi::Internal::RendererBackendServicesAccess::GetSwapChain(*m_backend);
	auto& descriptorHeapManager = Rhi::Internal::RendererBackendServicesAccess::GetDescriptorHeapManager(*m_backend);
	const FrameGraphDependencies
	    dependencies{rhi, *m_window, swapChain, descriptorHeapManager, ResolveSceneExtent(), ShouldPresentSceneToBackBuffer()};

	FrameGraphBuilder frameGraphBuilder(dependencies);
	FrameGraphBuildResult buildResult = frameGraphBuilder.Build();
	m_frameGraphSceneExtent = dependencies.sceneExtent;

	m_viewportRenderProducts.SceneColor.Handle =
	    buildResult.SceneColor.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.SceneColor.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_viewportRenderProducts.SceneDepth.Handle =
	    buildResult.SceneDepth.IsValid()
	        ? RenderProductHandle{static_cast<std::uint64_t>(buildResult.SceneDepth.GetResourceHandle().index) + 1ull}
	        : RenderProductHandle{};
	m_frameGraph = std::move(buildResult.Graph);
}

void Renderer::BindWindowResizeEvent() noexcept
{
	auto handle = m_window->OnResized.Add(
	    [this]()
	    {
		    m_bResizePending = true;
	    });
	m_resizeHandle = ScopedEventHandle(m_window->OnResized, handle);
}

void Renderer::RefreshFrameExecution() noexcept
{
	if (m_backend)
	{
		m_backend->Flush();
	}

	m_frameGraph.reset();
	InitializeFrameGraph();
}

void Renderer::BeginFrame() noexcept
{
	if (m_bResizePending)
	{
		m_bResizePending = false;

		if (m_window->HasValidSize())
		{
			m_backend->Flush();
			m_backend->ResizeSwapChain();
			RefreshFrameExecution();
		}
	}

	const RenderViewportExtent sceneExtent = ResolveSceneExtent();
	if (sceneExtent.Width != m_frameGraphSceneExtent.Width || sceneExtent.Height != m_frameGraphSceneExtent.Height)
	{
		RefreshFrameExecution();
	}

	m_backend->BeginFrame();
}

void Renderer::SetupFrame() noexcept
{
	m_timer->Tick();
	RefreshViewportRenderProducts();

	m_sceneSnapshot->Capture(*m_gameScene);
	m_textureManager->LoadSceneTextures(m_sceneSnapshot->textures);
	m_renderCamera->Update(m_sceneSnapshot->camera);

	m_backend->UpdatePerFrameConstants(static_cast<std::uint32_t>(CVarRenderViewMode.Get()));
}

void Renderer::RefreshViewportRenderProducts() noexcept
{
	const RenderProductHandle sceneColorHandle = m_viewportRenderProducts.SceneColor.Handle;
	const RenderProductHandle sceneDepthHandle = m_viewportRenderProducts.SceneDepth.Handle;
	const RenderViewportExtent extent = m_frameGraphSceneExtent.IsValid() ? m_frameGraphSceneExtent : ResolveSceneExtent();

	m_viewportRenderProducts = {};
	m_viewportRenderProducts.AvailableOutputs = RenderOutputFlags::SceneColor;
	m_viewportRenderProducts.SceneColor.Handle = sceneColorHandle;
	m_viewportRenderProducts.SceneColor.Extent = extent;
	m_viewportRenderProducts.SceneColor.Format = RenderProductFormat::ColorLdr;

	if (sceneDepthHandle)
	{
		m_viewportRenderProducts.SceneDepth.Handle = sceneDepthHandle;
		m_viewportRenderProducts.SceneDepth.Extent = extent;
		m_viewportRenderProducts.SceneDepth.Format = RenderProductFormat::DepthStencil;

		if (HasAnyRenderOutputFlags(m_viewportRenderRequest.RequestedOutputs, RenderOutputFlags::SceneDepth))
		{
			m_viewportRenderProducts.AvailableOutputs |= RenderOutputFlags::SceneDepth;
		}
	}
}

void Renderer::RecordFrame() noexcept
{
	auto& swapChain = Rhi::Internal::RendererBackendServicesAccess::GetSwapChain(*m_backend);
	auto& constantBufferManager = Rhi::Internal::RendererBackendServicesAccess::GetConstantBufferManager(*m_backend);
	auto& descriptorHeapManager = Rhi::Internal::RendererBackendServicesAccess::GetDescriptorHeapManager(*m_backend);
	auto& samplerLibrary = Rhi::Internal::RendererBackendServicesAccess::GetSamplerLibrary(*m_backend);
	FrameContext frame = BuildFrameContext(
	    *m_sceneSnapshot,
	    swapChain,
	    constantBufferManager,
	    *m_renderCamera,
	    *m_renderSceneDataBuilder,
	    *m_perViewDataBuilder,
	    *m_viewLightingBuilder,
	    *m_shadowFrameBuilder,
	    *m_shadowBuilder);

	m_frameGraph->Setup(frame);
	const FrameGraph::CompiledPlan compiledPlan = m_frameGraph->Compile();
	const RenderPassContext renderPassContext{
	    .DescriptorHeapManager = descriptorHeapManager,
	    .ConstantBufferManager = constantBufferManager,
	    .SamplerLibrary = samplerLibrary,
	    .RuntimeRegistry = m_pipelineStateManager->GetRuntimeRegistry()};

	const NativeGraphicsCommandListHandle commandListHandle = m_backend->GetCurrentGraphicsCommandListHandle();
	CommandContext cmd(static_cast<ID3D12GraphicsCommandList*>(commandListHandle.Value));
	m_frameGraph->Execute(compiledPlan, cmd, frame, renderPassContext);
}

void Renderer::SubmitFrame() noexcept
{
	m_backend->SubmitFrame();
}

void Renderer::EndFrame() noexcept
{
	m_backend->AdvanceFrameInFlight();
}

void Renderer::PostLoad() noexcept
{
	m_backend->CloseExecuteAndFlushCurrentFrame();
}

void Renderer::OnRender() noexcept
{
	PrepareHostFrame();
	RecordHostFrame();
	SubmitHostFrame();
}

Renderer::~Renderer() noexcept
{
	if (m_backend)
	{
		m_backend->Flush();
	}
}
