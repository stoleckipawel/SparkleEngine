#include "PCH.h"
#include "Scene/Lighting/LightRenderingControls.h"

#include "Core/Public/Console/CVar.h"

static ConsoleVariable<bool> CVarDirectionalLights("r.Lighting.Lights.Directional", true, "Evaluate directional lights.");
static ConsoleVariable<bool> CVarPointLights("r.Lighting.Lights.Point", true, "Evaluate point lights.");
static ConsoleVariable<bool> CVarSpotLights("r.Lighting.Lights.Spot", true, "Evaluate spot lights.");
static ConsoleVariable<bool> CVarRectLights("r.Lighting.Lights.Rect", true, "Evaluate rectangular area lights.");

bool IsLightRenderingEnabled(SceneLightKind kind) noexcept
{
	switch (kind)
	{
		case SceneLightKind::Directional:
			return CVarDirectionalLights.Get();
		case SceneLightKind::Point:
			return CVarPointLights.Get();
		case SceneLightKind::Spot:
			return CVarSpotLights.Get();
		case SceneLightKind::Rect:
			return CVarRectLights.Get();
		case SceneLightKind::Unknown:
			return false;
	}
	return false;
}
