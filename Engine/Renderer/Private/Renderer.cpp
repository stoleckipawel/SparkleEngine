#include "PCH.h"
#include "Renderer.h"

#include "Assets/AssetSystem.h"
#include "Assets/MaterialDesc.h"
#include "D3D12DebugLayer.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Rhi.h"
#include "D3D12SwapChain.h"
#include "Window.h"
#include "DxcShaderCompiler.h"
#include "ShaderCompileResult.h"
#include "TextureManager.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"
#include "Scene/Scene.h"
#include "Scene/Mesh.h"
#include "D3D12PipelineState.h"
#include "D3D12RootBindings.h"
#include "D3D12RootSignature.h"
#include "D3D12ConstantBuffer.h"
#include "D3D12ConstantBufferManager.h"
#include "D3D12ConstantBufferData.h"
#include "D3D12FrameResource.h"
#include "D3D12Texture.h"
#include "D3D12VertexLayout.h"
#include "Samplers/D3D12SamplerLibrary.h"
#include "D3D12DepthStencil.h"
#include "DepthConvention.h"
#include "UI.h"
#include "Time/Timer.h"
#include "Renderer/Public/Camera/RenderCamera.h"
#include "Renderer/Public/RenderContext.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/Passes/ForwardOpaquePass.h"
#include "Renderer/Public/Textures/MaterialFallbackTextures.h"
#include "Scene/Camera/GameCamera.h"
#include "SceneData/MaterialCacheUtils.h"
#include "SceneData/RendererMaterialCacheState.h"

Renderer::Renderer(Timer& timer, const AssetSystem& assetSystem, Scene& scene, Window& window) noexcept :
    m_timer(&timer),
    m_assetSystem(&assetSystem),
    m_scene(&scene),
    m_window(&window),
    m_materialCache(std::make_unique<RendererMaterialCacheState>())
{
	m_rhi = std::make_unique<D3D12Rhi>();

	constexpr UINT kInitFrameIndex = 0;
	m_rhi->SetCurrentFrameIndex(kInitFrameIndex);
	m_rhi->ResetCommandAllocator(kInitFrameIndex);
	m_rhi->ResetCommandList(kInitFrameIndex);

	m_rootSignature = std::make_unique<D3D12RootSignature>(*m_rhi);

	m_vertexShader = std::make_unique<ShaderCompileResult>(
	    DxcShaderCompiler::CompileFromAsset(*m_assetSystem, "Passes/Forward/ForwardLitVS.hlsl", ShaderStage::Vertex, "main"));
	m_pixelShader = std::make_unique<ShaderCompileResult>(
	    DxcShaderCompiler::CompileFromAsset(*m_assetSystem, "Passes/Forward/ForwardLitPS.hlsl", ShaderStage::Pixel, "main"));

	m_descriptorHeapManager = std::make_unique<D3D12DescriptorHeapManager>(*m_rhi);
	m_swapChain = std::make_unique<D3D12SwapChain>(*m_rhi, *m_window, *m_descriptorHeapManager);
	m_frameResourceManager = std::make_unique<D3D12FrameResourceManager>(*m_rhi, D3D12FrameResourceManager::DefaultCapacityPerFrame);

	m_ui = std::make_unique<UI>(*m_timer, *m_rhi, *m_window, *m_descriptorHeapManager, *m_swapChain);

	m_constantBufferManager = std::make_unique<D3D12ConstantBufferManager>(
	    *m_timer,
	    *m_rhi,
	    *m_window,
	    *m_descriptorHeapManager,
	    *m_frameResourceManager,
	    *m_swapChain,
	    *m_ui);

	m_samplerLibrary = std::make_unique<D3D12SamplerLibrary>(*m_rhi, *m_descriptorHeapManager);

	m_textureManager = std::make_unique<TextureManager>(*m_assetSystem, *m_rhi, *m_descriptorHeapManager);

	m_gpuMeshCache = std::make_unique<GPUMeshCache>(*m_rhi);

	SubscribeToDepthModeChanges();
	SubscribeToWindowResize();

	CreatePSO();

	CreateDepthStencilBuffer();

	m_renderCamera = std::make_unique<RenderCamera>(m_scene->GetCamera());

	m_frameGraph = std::make_unique<FrameGraph>(m_swapChain.get(), m_depthStencil.get());
	m_frameGraph->AddPass<ForwardOpaquePass>(
	    "ForwardOpaque",
	    *m_rootSignature,
	    *m_pso,
	    *m_constantBufferManager,
	    *m_descriptorHeapManager,
	    *m_textureManager,
	    *m_samplerLibrary,
	    *m_gpuMeshCache,
	    *m_swapChain,
	    *m_depthStencil);

	PostLoad();
}

void Renderer::PostLoad() noexcept
{
	m_rhi->CloseCommandList();
	m_rhi->ExecuteCommandList();
	m_rhi->Flush();
}

void Renderer::CreateDepthStencilBuffer()
{
	m_depthStencil = std::make_unique<D3D12DepthStencil>(*m_rhi, *m_window, *m_descriptorHeapManager);
}

void Renderer::OnResize() noexcept
{
	m_rhi->Flush();
	m_swapChain->Resize();
	CreateDepthStencilBuffer();
}

void Renderer::SubscribeToDepthModeChanges() noexcept
{
	auto handle = DepthConvention::OnModeChanged.Add(
	    [this](DepthMode mode)
	    {
		    OnDepthModeChanged(mode);
	    });
	m_depthModeChangedHandle = ScopedEventHandle(DepthConvention::OnModeChanged, handle);
}

void Renderer::SubscribeToWindowResize() noexcept
{
	auto handle = m_window->OnResized.Add(
	    [this]()
	    {
		    OnResize();
	    });
	m_resizeHandle = ScopedEventHandle(m_window->OnResized, handle);
}

void Renderer::OnRender() noexcept
{
	BeginFrame();
	SetupFrame();
	RecordFrame();
	SubmitFrame();
	EndFrame();
}

void Renderer::BeginFrame() noexcept
{
	const UINT frameIndex = m_swapChain->GetFrameInFlightIndex();
	m_rhi->SetCurrentFrameIndex(frameIndex);
	m_frameResourceManager->BeginFrame(m_rhi->GetFence().Get(), m_rhi->GetFenceEvent(), frameIndex);
	m_rhi->WaitForGPU(frameIndex);
	m_rhi->ResetCommandAllocator(frameIndex);
	m_rhi->ResetCommandList(frameIndex);
}

void Renderer::SetupFrame() noexcept
{
	m_renderCamera->Update();

	m_timer->Tick();
	m_ui->Update();
	m_constantBufferManager->UpdatePerFrame();
}

void Renderer::RecordFrame() noexcept
{
	SceneView sceneView = BuildSceneView();

	PerViewConstantBufferData viewData = m_renderCamera->GetViewConstantBufferData();
	viewData.SunDirection = sceneView.sunLight.direction;
	viewData.SunIntensity = sceneView.sunLight.intensity;
	viewData.SunColor = sceneView.sunLight.color;
	m_constantBufferManager->UpdatePerView(viewData);

	m_frameGraph->Setup(sceneView);

	m_frameGraph->Compile();

	RenderContext context(m_rhi->GetCommandList().Get());

	m_frameGraph->Execute(context);

	m_ui->Render();

	m_depthStencil->SetReadState();
	m_swapChain->SetPresentState();
}

void Renderer::SubmitFrame() noexcept
{
	m_rhi->CloseCommandList();
	m_rhi->ExecuteCommandList();
	m_rhi->Signal(m_swapChain->GetFrameInFlightIndex());

	m_frameResourceManager->EndFrame(m_rhi->GetNextFenceValue() - 1);
	m_swapChain->Present();
}

void Renderer::EndFrame() noexcept
{
	m_swapChain->UpdateFrameInFlightIndex();
}

SceneView Renderer::BuildSceneView() const
{
	SceneView view = {};
	InitializeSceneView(view);

	BuildMaterials(view);
	BuildMeshDraws(view);

	return view;
}

void Renderer::InitializeSceneView(SceneView& view) const
{
	view.width = m_window->GetWidth();
	view.height = m_window->GetHeight();

	view.camera = m_renderCamera.get();
}

void Renderer::BuildMaterials(SceneView& view) const
{
	if (!m_materialCache)
	{
		LOG_FATAL("Renderer::BuildMaterials: material cache state is unavailable.");
		return;
	}

	const auto& loadedMaterials = m_scene->GetLoadedMaterials();
	auto& materialCache = *m_materialCache;

	const bool shouldUseLoadedMaterials = !loadedMaterials.empty();
	const bool materialSetChanged = shouldUseLoadedMaterials
	                                    ? (!materialCache.materialCacheUsesLoadedMaterials ||
	                                       !MaterialCacheUtils::MaterialDescSetEquals(materialCache.cachedMaterialDescs, loadedMaterials))
	                                    : materialCache.materialCacheUsesLoadedMaterials;

	if (!materialCache.materialCacheBuilt || materialSetChanged)
	{
		RebuildMaterialCache();
	}

	if (!materialCache.cachedMaterialData.empty())
	{
		view.materials = materialCache.cachedMaterialData;
	}
}

void Renderer::RebuildMaterialCache() const
{
	if (!m_materialCache)
	{
		LOG_FATAL("Renderer::RebuildMaterialCache: material cache state is unavailable.");
		return;
	}

	const auto& loadedMaterials = m_scene->GetLoadedMaterials();
	auto& materialCache = *m_materialCache;

	ReleaseMaterialTextureTables();
	materialCache.cachedMaterialData.clear();
	materialCache.cachedMaterialDescs.clear();
	materialCache.materialCacheUsesLoadedMaterials = !loadedMaterials.empty();

	const auto* srvHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	if (!srvHeap)
	{
		LOG_FATAL("Renderer::RebuildMaterialCache: SRV heap is unavailable.");
		return;
	}

	auto buildMaterialTable = [this, srvHeap](const MaterialDesc& desc)
	{
		MaterialData material = MaterialData::FromDesc(desc);

		const D3D12Texture* textures[RootBindings::SRVRegister::MaterialTextureCount] = {
		    MaterialCacheUtils::ResolveMaterialTexture(*m_textureManager, desc.albedoTexture, MaterialFallbackTexture::Albedo),
		    MaterialCacheUtils::ResolveMaterialTexture(*m_textureManager, desc.normalTexture, MaterialFallbackTexture::Normal),
		    MaterialCacheUtils::ResolveMaterialTexture(
		        *m_textureManager,
		        desc.metallicRoughnessTexture,
		        MaterialFallbackTexture::MetallicRoughness),
		    MaterialCacheUtils::ResolveMaterialTexture(*m_textureManager, desc.occlusionTexture, MaterialFallbackTexture::Occlusion),
		    MaterialCacheUtils::ResolveMaterialTexture(*m_textureManager, desc.emissiveTexture, MaterialFallbackTexture::Emissive)};

		const D3D12DescriptorHandle tableHandle = m_descriptorHeapManager->AllocateContiguous(
		    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		    RootBindings::SRVRegister::MaterialTextureCount);

		for (std::uint32_t slot = 0; slot < RootBindings::SRVRegister::MaterialTextureCount; ++slot)
		{
			if (!textures[slot])
			{
				LOG_FATAL(std::format("Renderer::RebuildMaterialCache: Material texture slot {} resolved to null.", slot));
			}

			const D3D12_CPU_DESCRIPTOR_HANDLE destination = srvHeap->GetHandleAt(tableHandle.GetIndex() + slot).GetCPU();
			textures[slot]->WriteShaderResourceView(destination);
		}

		material.textureTableGpuHandle = tableHandle.GetGPU();
		m_materialCache->materialTextureTables.push_back(tableHandle);
		m_materialCache->cachedMaterialData.push_back(material);
	};

	if (!loadedMaterials.empty())
	{
		materialCache.cachedMaterialDescs = loadedMaterials;
		materialCache.cachedMaterialData.reserve(loadedMaterials.size());
		materialCache.materialTextureTables.reserve(loadedMaterials.size());

		for (const auto& desc : loadedMaterials)
		{
			buildMaterialTable(desc);
		}
	}
	else
	{
		materialCache.cachedMaterialData.reserve(1);
		materialCache.materialTextureTables.reserve(1);

		MaterialDesc defaultMaterial;
		defaultMaterial.name = "Renderer_DefaultMaterial";
		buildMaterialTable(defaultMaterial);
	}

	materialCache.materialCacheBuilt = true;
}

void Renderer::ReleaseMaterialTextureTables() const noexcept
{
	if (!m_materialCache)
	{
		return;
	}

	for (const D3D12DescriptorHandle& tableHandle : m_materialCache->materialTextureTables)
	{
		if (tableHandle.IsValid())
		{
			m_descriptorHeapManager->FreeContiguous(
			    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			    tableHandle,
			    RootBindings::SRVRegister::MaterialTextureCount);
		}
	}

	m_materialCache->materialTextureTables.clear();
}

void Renderer::BuildMeshDraws(SceneView& view) const
{
	if (!m_scene || !m_scene->HasMeshes())
		return;

	const auto& meshes = m_scene->GetMeshes();
	view.meshDraws.reserve(meshes.size());

	for (const auto& mesh : meshes)
	{
		MeshDraw draw = {};
		DirectX::XMStoreFloat4x4(&draw.worldMatrix, mesh->GetWorldMatrix());
		DirectX::XMStoreFloat3x4(&draw.worldInvTranspose, mesh->GetWorldInverseTransposeMatrix());
		draw.materialId = MaterialCacheUtils::ResolveMaterialId(mesh->GetMaterialId(), view.materials.size());
		draw.meshPtr = mesh.get();
		view.meshDraws.push_back(draw);
	}
}

Renderer::~Renderer() noexcept
{
	m_rhi->Flush();

	ReleaseMaterialTextureTables();
	if (m_materialCache)
	{
		m_materialCache->cachedMaterialData.clear();
		m_materialCache->cachedMaterialDescs.clear();
	}

	m_frameGraph.reset();

	m_renderCamera.reset();

	m_pso.reset();
	m_rootSignature.reset();
	m_depthStencil.reset();
	m_samplerLibrary.reset();
	m_textureManager.reset();

	m_constantBufferManager.reset();
	m_frameResourceManager.reset();
	m_swapChain.reset();
	m_ui.reset();
	m_descriptorHeapManager.reset();
}

void Renderer::CreatePSO()
{
	m_pso = std::make_unique<D3D12PipelineState>(
	    *m_rhi,
	    D3D12VertexLayout::GetStaticMeshLayout(),
	    *m_rootSignature,
	    m_vertexShader->GetBytecode(),
	    m_pixelShader->GetBytecode());
}

void Renderer::OnDepthModeChanged([[maybe_unused]] DepthMode mode) noexcept
{
	m_rhi->Flush();
	CreatePSO();
	CreateDepthStencilBuffer();
}
