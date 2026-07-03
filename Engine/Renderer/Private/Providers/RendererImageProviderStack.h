#pragma once

#include "RayReconstruction/RenderRayReconstructionPassServices.h"
#include "Upscaling/RenderUpscalingPassServices.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <memory>
#include <string_view>

class RayReconstructionSubsystem;
class RenderHardwareInterface;
class UpscalerSubsystem;
struct RayReconstructionInputContract;
struct UpscalerInputContract;

struct RendererImageProviderPassServices final
{
	RenderUpscalingPassServices Upscaling = {};
	RenderRayReconstructionPassServices RayReconstruction = {};
};

class RendererImageProviderStack final
{
  public:
	RendererImageProviderStack();
	~RendererImageProviderStack() noexcept;

	RendererImageProviderStack(const RendererImageProviderStack&) = delete;
	RendererImageProviderStack& operator=(const RendererImageProviderStack&) = delete;
	RendererImageProviderStack(RendererImageProviderStack&&) = delete;
	RendererImageProviderStack& operator=(RendererImageProviderStack&&) = delete;

	void Initialize(RenderHardwareInterface& renderHardware);
	void Refresh(RenderHardwareInterface& renderHardware);
	void Shutdown() noexcept;
	void OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent);
	void ResetHistory(std::string_view reason);
	void SetupUpscalerFrame(const UpscalerInputContract& inputContract);
	void SetupRayReconstructionFrame(const RayReconstructionInputContract& inputContract);

	std::uint32_t GetFrameGraphKey() const noexcept;
	RendererImageProviderPassServices BuildPassServices() noexcept;

  private:
	std::unique_ptr<UpscalerSubsystem> m_upscalerSubsystem;
	std::unique_ptr<RayReconstructionSubsystem> m_rayReconstructionSubsystem;
};
