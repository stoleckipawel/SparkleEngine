#pragma once

#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>

class IRayReconstructionProvider;
class RenderHardwareInterface;
class IUpscalerProvider;
struct ImageProviderFrameContext;

struct RendererImageProviderPassServices final
{
	IUpscalerProvider* Upscaling = nullptr;
	IRayReconstructionProvider* RayReconstruction = nullptr;
};

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
	explicit RendererImageProviderStack(RenderHardwareInterface& renderHardware);
	~RendererImageProviderStack() noexcept;

	RendererImageProviderStack(const RendererImageProviderStack&) = delete;
	RendererImageProviderStack& operator=(const RendererImageProviderStack&) = delete;
	RendererImageProviderStack(RendererImageProviderStack&&) = delete;
	RendererImageProviderStack& operator=(RendererImageProviderStack&&) = delete;

	void Refresh(RenderHardwareInterface& renderHardware);
	void ResetHistory() noexcept;
	void SetupFrame(const ImageProviderFrameContext& frameContext);
	RenderViewportExtent ResolveRenderExtent(
	    RenderViewportExtent outputExtent,
	    ImageProviderPipeline pipeline) noexcept;

	ImageProviderGraphKey GetFrameGraphKey() const noexcept;
	RendererImageProviderPassServices BuildPassServices() noexcept;

  private:
	void Initialize(RenderHardwareInterface& renderHardware);
	void Shutdown() noexcept;

	std::unique_ptr<IUpscalerProvider> m_upscaler;
	std::unique_ptr<IRayReconstructionProvider> m_rayReconstruction;
	bool m_rayReconstructionRequested = false;
	bool m_resetHistoryPending = true;
};
