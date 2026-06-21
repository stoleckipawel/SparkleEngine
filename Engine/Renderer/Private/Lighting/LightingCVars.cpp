#include "PCH.h"

#include "Lighting/LightingCVars.h"

#include "RHI/Public/Resources/RenderLightingLimits.h"

ConsoleVariable<std::uint32_t> CVarMaxDirectionalLights(
    "r.Lighting.MaxDirectionalLights",
    static_cast<std::uint32_t>(RenderLightingLimits::MaxDirectionalLights),
    "Maximum active directional lights submitted to lighting shaders.");

ConsoleVariable<std::uint32_t> CVarMaxPointLights(
    "r.Lighting.MaxPointLights",
    static_cast<std::uint32_t>(RenderLightingLimits::MaxPointLights),
    "Maximum active point lights submitted to lighting shaders.");

ConsoleVariable<std::uint32_t> CVarMaxSpotLights(
    "r.Lighting.MaxSpotLights",
    static_cast<std::uint32_t>(RenderLightingLimits::MaxSpotLights),
    "Maximum active spot lights submitted to lighting shaders.");
