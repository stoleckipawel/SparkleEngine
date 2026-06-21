#pragma once

#include "Device/RenderDeviceBackendServices.h"

#include <memory>

class Window;

std::unique_ptr<RenderDeviceBackendServices> CreateD3D12RenderDeviceServices(Window& window) noexcept;
std::unique_ptr<RenderDeviceBackendServices> CreateVulkanRenderDeviceServices(Window& window) noexcept;
