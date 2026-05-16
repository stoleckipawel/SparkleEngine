#pragma once

#include "Device/RenderDeviceBackendServices.h"

#include <memory>

class Timer;
class Window;

std::unique_ptr<RenderDeviceBackendServices> CreateD3D12RenderDeviceServices(Timer& timer, Window& window) noexcept;