#pragma once

#include "Frame/RhiFrameConstants.h"
#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "Device/RenderHardwareInterface.h"

#include <array>
#include <memory>
#include <string>

class D3D12DescriptorHeapManager;
class D3D12DescriptorService;
class D3D12UploadService;
class D3D12CaptureService;
class D3D12ImGuiBackend;
class D3D12InteropService;
class D3D12GpuMemoryAllocator;
class D3D12PipelineService;
class D3D12PresentationService;
class D3D12RenderCommandList;
class D3D12ResourceService;
class D3D12Rhi;
class D3D12RayTracingServices;
class D3D12SamplerLibrary;
class D3D12SwapChain;
class RhiImGuiRenderer;

class D3D12RenderHardwareInterface final : public RenderHardwareInterface
{
  public:
	D3D12RenderHardwareInterface(
	    D3D12Rhi& rhi,
	    D3D12GpuMemoryAllocator& memoryAllocator,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    D3D12SwapChain& swapChain,
	    D3D12UploadService& uploadService) noexcept;
	~D3D12RenderHardwareInterface() noexcept;

	D3D12RenderHardwareInterface(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface& operator=(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface(D3D12RenderHardwareInterface&&) = delete;
	D3D12RenderHardwareInterface& operator=(D3D12RenderHardwareInterface&&) = delete;

	const RhiCapabilities& GetCapabilities() const noexcept { return m_capabilities; }
	ERhiBackendApi GetBackendApi() const noexcept;
	CookedShaderBinaryFormat GetRequiredShaderBinaryFormat() const noexcept;
	std::uint32_t GetCurrentFrameIndex() const noexcept;
	RhiResourceService& GetResourceService() noexcept;
	const RhiResourceService& GetResourceService() const noexcept;
	RhiDescriptorService& GetDescriptorService() noexcept;
	const RhiDescriptorService& GetDescriptorService() const noexcept;
	RhiPipelineService& GetPipelineService() noexcept;
	RhiUploadService& GetUploadService() noexcept;
	const RhiUploadService& GetUploadService() const noexcept;
	RhiRayTracingService& GetRayTracingService() noexcept;
	const RhiRayTracingService& GetRayTracingService() const noexcept;
	void WaitForIdle() noexcept;
	RhiInteropService& GetInteropService() noexcept;
	const RhiInteropService& GetInteropService() const noexcept;
	RhiCaptureService& GetCaptureService() noexcept;
	RenderDiagnostics& GetDiagnostics() noexcept;
	const RenderDiagnostics& GetDiagnostics() const noexcept;
	RhiPresentationService& GetPresentationService() noexcept;
	const RhiPresentationService& GetPresentationService() const noexcept;
	NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept;
	NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept;
	RenderCommandList& GetCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	ID3D12DescriptorHeap* GetD3D12ShaderResourceDescriptorHeap() const noexcept;
	RhiViewport GetBackBufferViewport() const noexcept;
	RhiRect GetBackBufferScissorRect() const noexcept;
	RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept;
	NativeResourceHandle GetBackBufferResource() const noexcept;
	RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept;
	RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount,
	    ERhiClassicTlasBuildFlags buildFlags = ERhiClassicTlasBuildFlags::None) const noexcept;
	RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName);
	std::uint64_t ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept;
	void BeginPresentRenderPass(const float clearColor[4]) noexcept;
	void BeginPresentOverlayPass() noexcept;
	void EndPresentRenderPass() noexcept;
	PixelFormat GetPresentColorFormat() const noexcept;
	PixelFormat GetPresentDepthStencilFormat() const noexcept;
	void SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept;

  private:
	friend class D3D12RenderCommandList;

	D3D12_CPU_DESCRIPTOR_HANDLE ResolveDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE ResolveDescriptorTableGpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	void BeginResourceTracking(NativeResourceHandle resource) noexcept;
	void EndResourceTracking(NativeResourceHandle resource, RhiSubmissionToken submissionToken) noexcept;
	bool BuildPartitionedTopLevelAccelerationStructure(
	    ID3D12GraphicsCommandList7* commandList,
	    const RhiPartitionedTlasBuildCommandDesc& desc) const noexcept;
	RhiCapabilities BuildCapabilities() const noexcept;
	RhiFormatSupport QueryFormatSupport(PixelFormat format) const noexcept;

	std::unique_ptr<D3D12InteropService> m_interopService;
	std::unique_ptr<D3D12CaptureService> m_captureService;
	std::unique_ptr<D3D12PresentationService> m_presentationService;
	std::unique_ptr<D3D12PipelineService> m_pipelineService;
	std::unique_ptr<D3D12ResourceService> m_resourceService;
	std::unique_ptr<D3D12RayTracingServices> m_rayTracingServices;
	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	D3D12UploadService* m_uploadService = nullptr;
	std::unique_ptr<D3D12DescriptorService> m_descriptorService;
	std::unique_ptr<D3D12ImGuiBackend> m_imguiBackend;
	RhiCapabilities m_capabilities;
	std::unique_ptr<RenderDiagnostics> m_diagnostics;
	std::array<std::array<std::unique_ptr<RenderCommandList>, RhiFrameConstants::FramesInFlight>, RhiQueueTypeCount> m_commandLists;
};
