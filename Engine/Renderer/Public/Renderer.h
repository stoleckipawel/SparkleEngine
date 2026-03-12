#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/SceneView.h"

#include "Event.h"
#include "Events/ScopedEventHandle.h"
#include <cstdint>
#include <memory>
#include <vector>

enum class DepthMode : std::uint8_t;

class Timer;

class AssetSystem;
class D3D12Rhi;
class D3D12Texture;
class D3D12PipelineState;
class D3D12RootSignature;
class D3D12SamplerLibrary;
class D3D12DepthStencil;
class D3D12ConstantBufferManager;
class D3D12DescriptorHeapManager;
class D3D12FrameResourceManager;
class D3D12SwapChain;
class FrameGraph;
class GPUMeshCache;
class RenderCamera;
class Scene;
class Window;
class UI;
class TextureManager;
class ShaderCompileResult;
struct RendererMaterialCacheState;

class SPARKLE_RENDERER_API Renderer final
{
  public:
	Renderer(Timer& timer, const AssetSystem& assetSystem, Scene& scene, Window& window) noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void OnRender() noexcept;

  private:
	void PostLoad() noexcept;
	void CreateDepthStencilBuffer();
	void CreatePSO();
	void OnDepthModeChanged(DepthMode mode) noexcept;
	void OnResize() noexcept;
	void SubscribeToDepthModeChanges() noexcept;
	void SubscribeToWindowResize() noexcept;

	void BeginFrame() noexcept;
	void SetupFrame() noexcept;
	void RecordFrame() noexcept;
	void SubmitFrame() noexcept;
	void EndFrame() noexcept;

	SceneView BuildSceneView() const;

	void InitializeSceneView(SceneView& view) const;

	void BuildMaterials(SceneView& view) const;
	void RebuildMaterialCache() const;
	void ReleaseMaterialTextureTables() const noexcept;

	void BuildMeshDraws(SceneView& view) const;

	Timer* m_timer = nullptr;

	const AssetSystem* m_assetSystem = nullptr;

	std::unique_ptr<D3D12Rhi> m_rhi;

	std::unique_ptr<GPUMeshCache> m_gpuMeshCache;

	std::unique_ptr<TextureManager> m_textureManager;

	std::unique_ptr<D3D12DepthStencil> m_depthStencil;

	std::unique_ptr<D3D12SamplerLibrary> m_samplerLibrary;

	std::unique_ptr<D3D12PipelineState> m_pso;
	std::unique_ptr<D3D12RootSignature> m_rootSignature;

	std::unique_ptr<ShaderCompileResult> m_vertexShader;
	std::unique_ptr<ShaderCompileResult> m_pixelShader;

	std::unique_ptr<RenderCamera> m_renderCamera;

	std::unique_ptr<D3D12ConstantBufferManager> m_constantBufferManager;

	std::unique_ptr<D3D12FrameResourceManager> m_frameResourceManager;

	std::unique_ptr<D3D12DescriptorHeapManager> m_descriptorHeapManager;

	std::unique_ptr<D3D12SwapChain> m_swapChain;

	std::unique_ptr<UI> m_ui;

	std::unique_ptr<FrameGraph> m_frameGraph;

	Scene* m_scene = nullptr;

	Window* m_window = nullptr;

	ScopedEventHandle m_depthModeChangedHandle;
	ScopedEventHandle m_resizeHandle;

	mutable std::unique_ptr<RendererMaterialCacheState> m_materialCache;
};
