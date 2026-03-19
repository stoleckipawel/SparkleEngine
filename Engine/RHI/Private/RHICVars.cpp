#include "PCH.h"

#include "RHI/Public/RHICVars.h"

ConsoleVariable<bool> CVarRhiVSync("r.VSync", true, "Enable vertical sync during swap chain present.");

ConsoleVariable<bool> CVarRhiPreferHighPerformanceAdapter(
    "r.PreferHighPerformanceAdapter",
    true,
    "Prefer the high-performance GPU when selecting a DXGI adapter.");