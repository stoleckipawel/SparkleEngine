#include "PCH.h"
#include "Passes/Lighting/Shadows/DirectShadowControls.h"

#include "Core/Public/Console/CVar.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"

ConsoleVariable<bool> CVarDirectShadows("r.Lighting.Shadows.Direct", true, "Evaluate primary direct-light shadow visibility.");

bool IsDirectShadowsActive() noexcept
{
	return CVarDirectShadows.Get() && IsDirectLightingAdmitted();
}

void RequireDirectShadowSignal(bool available) noexcept
{
	if (!available)
	{
		SPARKLE_DEFINE_LOG_CATEGORY_STATIC(LogDirectShadows, "Renderer.DirectShadows");
		Diagnostics::Fatal(LogDirectShadows, __FILE__, __LINE__, "Active direct shadows require their visibility signal.");
	}
}
