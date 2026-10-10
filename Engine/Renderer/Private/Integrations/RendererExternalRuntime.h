#pragma once

#include "Core/Public/Threading/ThreadOwnership.h"
#include "RHI/Public/Device/RhiDeviceLaunch.h"
#include "Renderer/Public/ExternalCapture/RendererGraphicsLaunch.h"
#include <memory>

#include <cstdint>

// Owns process-facing renderer integration initialization on the application
// thread. RenderCoordinator receives only the immutable device bootstrap value.
class RendererExternalRuntime final
{
public:
	explicit RendererExternalRuntime(RendererGraphicsLaunch graphicsLaunch) noexcept;
	~RendererExternalRuntime() noexcept;

	RendererExternalRuntime(const RendererExternalRuntime&) = delete;
	RendererExternalRuntime& operator=(const RendererExternalRuntime&) = delete;

	const RhiDeviceLaunch& GetDeviceLaunch() const noexcept;
	void BeginSimulationFrame(std::uint64_t frameId) noexcept;
	void EndSimulationFrame(std::uint64_t frameId) noexcept;

private:
	Threading::OwnerThread m_owner{"Renderer external runtime"};

	RhiDeviceLaunch m_deviceLaunch;
#if SPARKLE_WITH_EXTERNAL_CAPTURE
	std::unique_ptr<RhiExternalCapture> m_externalCapture;
#endif
};
