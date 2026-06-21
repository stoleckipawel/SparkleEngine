#pragma once

#include "Device/RenderDeviceBackendServices.h"
#include "Device/RenderDeviceSettings.h"

#include <memory>

class Window;

std::unique_ptr<RenderDeviceBackendServices> CreateD3D12RenderDeviceServices(
    Window& window,
    const RenderDeviceSettings& settings) noexcept;
std::unique_ptr<RenderDeviceBackendServices> CreateVulkanRenderDeviceServices(
    Window& window,
    const RenderDeviceSettings& settings) noexcept;
