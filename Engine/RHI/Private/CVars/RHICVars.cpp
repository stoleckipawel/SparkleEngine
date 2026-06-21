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
    RhiPresentationDefaults::BackBufferFormat,
    "Preferred swap chain color format. Device lifetime setting; recreate the renderer to apply.");
