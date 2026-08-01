#pragma once

#include "Device/RenderDeviceBackendServices.h"
#include "Formats/PixelFormat.h"
#include "Interop/RhiD3D12InterposerHooks.h"
#include "Presentation/RhiPresentationDefaults.h"

#include <memory>

class Window;

std::unique_ptr<RenderDeviceBackendServices> CreateD3D12RenderDeviceServices(
	Window& window,
	PixelFormat backBufferFormat,
	const RhiPresentationConfiguration& presentationConfiguration,
	RhiD3D12InterposerHooks interposerHooks) noexcept;
std::unique_ptr<RenderDeviceBackendServices> CreateVulkanRenderDeviceServices(
	Window& window,
	PixelFormat backBufferFormat,
	const RhiPresentationConfiguration& presentationConfiguration) noexcept;
