#include "../../../PCH.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowCVars.h"

ConsoleVariable<bool> CVarRayTracedShadowsEnabled(
    "r.RayTracedShadows.Enabled",
    true,
    "Enable ray traced shadow ray queries.");
ConsoleVariable<float> CVarRayTracedShadowNormalBias(
	"r.RayTracedShadows.NormalBias",
	0.01f,
	"World-space normal offset used by ray traced shadow rays.");
ConsoleVariable<float> CVarRayTracedShadowMaxDistance(
	"r.RayTracedShadows.MaxDistance",
	100000.0f,
	"Maximum ray distance for directional ray traced shadows.");
ConsoleVariable<bool> CVarRayTracedShadowDiagnosticsEnabled(
	"r.RayTracedShadows.Diagnostics",
	false,
	"Enable additional ray traced shadow diagnostics.");
