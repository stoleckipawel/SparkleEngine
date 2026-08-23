#pragma once

#include "Viewport/ViewportContracts.h"
#include "RHI/Public/Commands/RhiQueue.h"

#include <cstdint>
#include <memory>
#include <vector>

class IRayReconstructionProvider;
class RenderDeviceServices;
class RenderHardwareInterface;
class IUpscalerProvider;
struct ImageProviderFrameInput;

struct ImageProviderGraphKey final
{
	std::uint32_t UpscalerProvider = 0;
	std::uint32_t RayReconstructionMode = 0;

	bool operator==(const ImageProviderGraphKey&) const noexcept = default;
};

enum class ImageProviderPipeline : std::uint8_t
{
	PresentationUpscaling,
	RayReconstruction
};

class RendererImageProviderStack final
{
public:
	RendererImageProviderStack(RenderHardwareInterface& renderHardwareInterface, RenderDeviceServices& deviceServices);
	~RendererImageProviderStack() noexcept;

	RendererImageProviderStack(const RendererImageProviderStack&) = delete;
	RendererImageProviderStack& operator=(const RendererImageProviderStack&) = delete;
	RendererImageProviderStack(RendererImageProviderStack&&) = delete;
	RendererImageProviderStack& operator=(RendererImageProviderStack&&) = delete;

	void ResetHistory() noexcept;
	void Refresh() noexcept;
	void PollRetiredGenerations() noexcept;
	void SetupFrame(const ImageProviderFrameInput& frameInput);
	RenderViewportExtent ResolveRenderExtent(RenderViewportExtent outputExtent, ImageProviderPipeline pipeline) noexcept;

	ImageProviderGraphKey GetFrameGraphKey() const noexcept;
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	IUpscalerProvider* GetUpscalerProvider() noexcept { return m_upscaler.get(); }
	IRayReconstructionProvider* GetRayReconstructionProvider() noexcept { return m_rayReconstruction.get(); }

private:
	void Initialize();
	void Shutdown() noexcept;
	static void ShutdownProviders(
	    std::unique_ptr<IUpscalerProvider>& upscaler,
	    std::unique_ptr<IRayReconstructionProvider>& rayReconstruction) noexcept;

	struct RetiredGeneration final
	{
		RhiSubmissionState LastUse;
		std::unique_ptr<IUpscalerProvider> Upscaler;
		std::unique_ptr<IRayReconstructionProvider> RayReconstruction;
	};

	RenderHardwareInterface& m_renderHardwareInterface;
	RenderDeviceServices& m_deviceServices;
	std::unique_ptr<IUpscalerProvider> m_upscaler;
	std::unique_ptr<IRayReconstructionProvider> m_rayReconstruction;
	std::vector<RetiredGeneration> m_retiredGenerations;
	std::uint64_t m_generation = 1;
	bool m_resetHistoryPending = true;
};
