#pragma once

#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>

class IRayReconstructionProvider;
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
	explicit RendererImageProviderStack(RenderHardwareInterface& renderHardwareInterface);
	~RendererImageProviderStack() noexcept;

	RendererImageProviderStack(const RendererImageProviderStack&) = delete;
	RendererImageProviderStack& operator=(const RendererImageProviderStack&) = delete;
	RendererImageProviderStack(RendererImageProviderStack&&) = delete;
	RendererImageProviderStack& operator=(RendererImageProviderStack&&) = delete;

	void ResetHistory() noexcept;
	void SetupFrame(const ImageProviderFrameInput& frameInput);
	RenderViewportExtent ResolveRenderExtent(RenderViewportExtent outputExtent, ImageProviderPipeline pipeline) noexcept;

	ImageProviderGraphKey GetFrameGraphKey() const noexcept;
	IUpscalerProvider* GetUpscalerProvider() noexcept { return m_upscaler.get(); }
	IRayReconstructionProvider* GetRayReconstructionProvider() noexcept { return m_rayReconstruction.get(); }

private:
	void Initialize(RenderHardwareInterface& renderHardwareInterface);
	void Shutdown() noexcept;

	std::unique_ptr<IUpscalerProvider> m_upscaler;
	std::unique_ptr<IRayReconstructionProvider> m_rayReconstruction;
	bool m_resetHistoryPending = true;
};
