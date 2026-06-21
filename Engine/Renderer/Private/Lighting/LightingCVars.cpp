#include "PCH.h"

#include "Lighting/LightingCVars.h"

ConsoleVariable<std::uint32_t> CVarMaxDirectionalLights(
    "r.Lighting.MaxDirectionalLights",
    2u,
    "Maximum active directional lights submitted to lighting shaders.");

ConsoleVariable<std::uint32_t> CVarMaxPointLights(
    "r.Lighting.MaxPointLights",
    512u,
    "Maximum active point lights submitted to lighting shaders.");

ConsoleVariable<std::uint32_t> CVarMaxSpotLights(
    "r.Lighting.MaxSpotLights",
    512u,
    "Maximum active spot lights submitted to lighting shaders.");
