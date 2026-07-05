#pragma once

#include "Device/RenderDeviceBackendServices.h"
#include "Formats/PixelFormat.h"

#include <memory>

class Window;

std::unique_ptr<RenderDeviceBackendServices> CreateD3D12RenderDeviceServices(
    Window& window,
    PixelFormat backBufferFormat) noexcept;
std::unique_ptr<RenderDeviceBackendServices> CreateVulkanRenderDeviceServices(
    Window& window,
    PixelFormat backBufferFormat) noexcept;
