#include "PCH.h"
#include "Passes/Lighting/Shadows/DirectShadowControls.h"

#include "Core/Public/Console/CVar.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Passes/Lighting/Direct/DirectLightingControls.h"

static ConsoleVariable<bool> CVarDirectShadows("r.Lighting.Shadows.Direct", true, "Evaluate primary direct-light shadow visibility.");

bool IsDirectShadowsEnabled() noexcept
{
	return CVarDirectShadows.Get();
}

bool IsDirectShadowsActive() noexcept
{
	return IsDirectShadowsEnabled() && IsDirectLightingAdmitted();
}

void RequireDirectShadowSignal(bool available) noexcept
{
	if (!available)
	{
		SPARKLE_DEFINE_LOG_CATEGORY_STATIC(LogDirectShadows, "Renderer.DirectShadows");
		Diagnostics::Fatal(LogDirectShadows, __FILE__, __LINE__, "Active direct shadows require their visibility signal.");
	}
}

std::uint64_t AppendDirectShadowHistoryInvalidationHash(std::uint64_t hash) noexcept
{
	return Hash::ContinueFnv1a64Value(hash, IsDirectShadowsEnabled());
}
