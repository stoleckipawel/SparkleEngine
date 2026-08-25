#include "PCH.h"
#include "Scene/Materials/MaterialCVars.h"

ConsoleVariable<MaterialBindingMode> CVarRendererMaterialBindingMode(
    "r.Material.BindingMode",
    MaterialBindingMode::RayTracingOnly,
    "Renderer material binding mode: 0=RayTracingOnly, 1=Everything.");
