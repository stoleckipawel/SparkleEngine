#include "PCH.h"
#include "Scene/Materials/MaterialCVars.h"

ConsoleVariable<MaterialBindingMode> CVarRendererMaterialBindingMode(
    "r.Material.BindingMode",
    MaterialBindingMode::RaytracingOnly,
    "Renderer material binding mode: 0=RaytracingOnly, 1=Everything.");
