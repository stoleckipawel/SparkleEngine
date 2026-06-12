#include "Vulkan/VulkanPCH.h"

#include "Device/RenderDeviceBackendFactory.h"

#include "Config/RenderConfig.h"
#include "Vulkan/Commands/VulkanCommandContext.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/VulkanRenderHardwareInterface.h"

#include "Time/Timer.h"
#include "Window/Window.h"

#include "Core/Public/Diagnostics/Trace.h"

class VulkanRenderDeviceServices final : public RenderDeviceBackendServices
{
  public:
	static std::unique_ptr<VulkanRenderDeviceServices> Create(Timer& timer, Window& window) noexcept;
	~VulkanRenderDeviceServices() noexcept override;

	VulkanRenderDeviceServices(const VulkanRenderDeviceServices&) = delete;
	VulkanRenderDeviceServices& operator=(const VulkanRenderDeviceServices&) = delete;
	VulkanRenderDeviceServices(VulkanRenderDeviceServices&&) = delete;
	VulkanRenderDeviceServices& operator=(VulkanRenderDeviceServices&&) = delete;

	RenderHardwareInterface& GetRenderHardwareInterface() noexcept override;
	const RenderHardwareInterface& GetRenderHardwareInterface() const noexcept override;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept override;
	RenderDiagnostics& GetDiagnostics() noexcept override;
	const RenderDiagnostics& GetDiagnostics() const noexcept override;
	void Flush() noexcept override;
	void ResizeSwapChain() noexcept override;
	void BeginFrame() noexcept override;
	RenderCommandList& GetCurrentGraphicsCommandList() noexcept override;
	void SubmitFrame() noexcept override;
	void AdvanceFrameInFlight() noexcept override;
	void UpdatePerFrameConstants(std::uint32_t renderViewMode, std::uint32_t viewportWidth, std::uint32_t viewportHeight) noexcept override;
	void CloseExecuteAndFlushCurrentFrame() noexcept override;

  private:
	VulkanRenderDeviceServices() noexcept = default;

	std::unique_ptr<VulkanRhi> m_rhi;
	std::unique_ptr<VulkanGpuMemoryAllocator> m_memoryAllocator;
	std::unique_ptr<VulkanSwapChain> m_swapChain;
	std::unique_ptr<VulkanCommandContext> m_commandContext;
	std::unique_ptr<VulkanRenderHardwareInterface> m_renderHardwareInterface;
	Timer* m_timer = nullptr;
	Window* m_window = nullptr;
	std::uint32_t m_currentFrameIndex = 0;
	bool m_hasAcquiredBackBuffer = false;
};

std::unique_ptr<RenderDeviceBackendServices> CreateVulkanRenderDeviceServices(Timer& timer, Window& window) noexcept
{
	return VulkanRenderDeviceServices::Create(timer, window);
}

std::unique_ptr<VulkanRenderDeviceServices> VulkanRenderDeviceServices::Create(Timer& timer, Window& window) noexcept
{
	auto services = std::unique_ptr<VulkanRenderDeviceServices>(new VulkanRenderDeviceServices());
	services->m_timer = &timer;
	services->m_window = &window;
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.CreateDevice");
		services->m_rhi = std::make_unique<VulkanRhi>();
	}
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.CreateMemoryAllocator");
		services->m_memoryAllocator = std::make_unique<VulkanGpuMemoryAllocator>(*services->m_rhi);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.CreateSwapChain");
		services->m_swapChain = std::make_unique<VulkanSwapChain>(*services->m_rhi, window);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.CreateCommandContext");
		services->m_commandContext = std::make_unique<VulkanCommandContext>(*services->m_rhi);
	}
	{
		SPARKLE_CPU_SCOPE("RHI.Vulkan.CreateHardwareInterface");
		services->m_renderHardwareInterface = std::make_unique<VulkanRenderHardwareInterface>(
		    *services->m_rhi,
		    *services->m_swapChain,
		    *services->m_commandContext,
		    *services->m_memoryAllocator);
	}
	return services;
}

VulkanRenderDeviceServices::~VulkanRenderDeviceServices() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		m_renderHardwareInterface->WaitForIdle();
	}

	m_renderHardwareInterface.reset();
	m_commandContext.reset();
	m_swapChain.reset();
	m_memoryAllocator.reset();
	m_rhi.reset();
}

RenderHardwareInterface& VulkanRenderDeviceServices::GetRenderHardwareInterface() noexcept
{
	return *m_renderHardwareInterface;
}

const RenderHardwareInterface& VulkanRenderDeviceServices::GetRenderHardwareInterface() const noexcept
{
	return *m_renderHardwareInterface;
}

RhiImGuiRenderer& VulkanRenderDeviceServices::GetImGuiRenderer() noexcept
{
	return m_renderHardwareInterface->GetImGuiRenderer();
}

RenderDiagnostics& VulkanRenderDeviceServices::GetDiagnostics() noexcept
{
	return m_renderHardwareInterface->GetDiagnostics();
}

const RenderDiagnostics& VulkanRenderDeviceServices::GetDiagnostics() const noexcept
{
	return m_renderHardwareInterface->GetDiagnostics();
}

void VulkanRenderDeviceServices::Flush() noexcept
{
	m_hasAcquiredBackBuffer = false;
	m_renderHardwareInterface->WaitForIdle();
}

void VulkanRenderDeviceServices::ResizeSwapChain() noexcept
{
	m_renderHardwareInterface->WaitForIdle();
	m_swapChain->Resize();
	m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
}

void VulkanRenderDeviceServices::BeginFrame() noexcept
{
	m_renderHardwareInterface->SetCurrentFrameIndex(m_currentFrameIndex);
	m_commandContext->BeginFrame(m_currentFrameIndex);
	m_renderHardwareInterface->ResetTransientFrameResources();
	m_hasAcquiredBackBuffer = m_swapChain->AcquireNextImage(m_commandContext->GetImageAvailableSemaphore(m_currentFrameIndex));
	if (!m_hasAcquiredBackBuffer)
	{
		m_commandContext->CancelFrame(m_currentFrameIndex);
		m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
	}
}

RenderCommandList& VulkanRenderDeviceServices::GetCurrentGraphicsCommandList() noexcept
{
	return m_renderHardwareInterface->GetGraphicsCommandList(m_currentFrameIndex);
}

void VulkanRenderDeviceServices::SubmitFrame() noexcept
{
	if (!m_hasAcquiredBackBuffer)
	{
		return;
	}

	const VkSemaphore imageAvailableSemaphore = m_commandContext->GetImageAvailableSemaphore(m_currentFrameIndex);
	const VkSemaphore renderFinishedSemaphore = m_swapChain->GetCurrentRenderFinishedSemaphore();
	m_commandContext->SubmitFrame(m_currentFrameIndex, imageAvailableSemaphore, renderFinishedSemaphore);
	if (m_swapChain->Present(renderFinishedSemaphore))
	{
		m_renderHardwareInterface->RebuildSwapChainBackBufferViews();
	}
	m_hasAcquiredBackBuffer = false;
}

void VulkanRenderDeviceServices::AdvanceFrameInFlight() noexcept
{
	m_currentFrameIndex = (m_currentFrameIndex + 1u) % RenderConfig::FramesInFlight;
	m_renderHardwareInterface->SetCurrentFrameIndex(m_currentFrameIndex);
}

void VulkanRenderDeviceServices::UpdatePerFrameConstants(
    std::uint32_t renderViewMode,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight) noexcept
{
	if (m_renderHardwareInterface == nullptr || m_timer == nullptr)
	{
		return;
	}

	PerFrameConstantBufferData data = {};
	data.FrameIndex = m_timer->GetFrameCount();
	data.TotalTime = static_cast<float>(m_timer->GetTotalTime(TimeDomain::Unscaled, TimeUnit::Seconds));
	data.DeltaTime = static_cast<float>(m_timer->GetDelta(TimeDomain::Unscaled, TimeUnit::Seconds));
	data.ScaledTotalTime = static_cast<float>(m_timer->GetTotalTime(TimeDomain::Scaled, TimeUnit::Seconds));
	data.ScaledDeltaTime = static_cast<float>(m_timer->GetDelta(TimeDomain::Scaled, TimeUnit::Seconds));
	const float width = static_cast<float>(viewportWidth);
	const float height = static_cast<float>(viewportHeight);
	data.ViewModeIndex = renderViewMode;
	data.ViewportSize = DirectX::XMFLOAT2(width, height);
	data.ViewportSizeInv = DirectX::XMFLOAT2(width != 0.0f ? 1.0f / width : 0.0f, height != 0.0f ? 1.0f / height : 0.0f);

	m_renderHardwareInterface->UpdatePerFrameConstants(data);
}

void VulkanRenderDeviceServices::CloseExecuteAndFlushCurrentFrame() noexcept
{
	if (m_hasAcquiredBackBuffer)
	{
		SubmitFrame();
	}
	m_renderHardwareInterface->WaitForIdle();
}
