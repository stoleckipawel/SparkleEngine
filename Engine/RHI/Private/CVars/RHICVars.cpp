#include "PCH.h"

#include "CVars/RHICVars.h"
#include "Presentation/RhiPresentationDefaults.h"

ConsoleVariable<bool> CVarVSync("r.VSync", true, "Enable vertical sync during swap chain present.");

ConsoleVariable<bool> CVarPreferHighPerformanceAdapter(
    "r.PreferHighPerformanceAdapter",
    true,
    "Prefer the high-performance GPU when selecting a DXGI adapter.");

ConsoleVariable<bool> CVarRayTracingPreferPartitionedTlas(
    "r.RayTracing.PreferPartitionedTlas",
    false,
    "Prefer partitioned top-level acceleration structures when a backend provider is available.");

ConsoleVariable<PixelFormat> CVarBackBufferFormat(
    "r.BackBufferFormat",
    RhiPresentationDefaults::DefaultBackBufferFormat,
    "Preferred swap chain color format. Device lifetime setting; recreate the renderer to apply.");

ConsoleVariable<std::uint32_t> CVarBackBufferCount(
    "r.BackBufferCount",
    RhiPresentationDefaults::DefaultBackBufferCount,
    "Swap chain image count. Supported values are 2 and 3; recreate the renderer to apply.");

ConsoleVariable<std::uint32_t> CVarMaximumFramesInFlight(
    "r.MaximumFramesInFlight",
    RhiPresentationDefaults::DefaultMaximumFramesInFlight,
    "Maximum submitted frames awaiting GPU retirement. Supported values are 1 through 3 and cannot exceed r.BackBufferCount; recreate the "
    "renderer to apply.");
