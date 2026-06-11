#include "../PCH.h"
#include "Upscaling/UpscalerSettings.h"

#include "Core/Public/Console/CVar.h"

namespace
{
	ConsoleVariable<EUpscalerProviderKind> CVarUpscalerProvider(
	    "r.Upscaler.Provider",
	    EUpscalerProviderKind::Passthrough,
	    "Renderer upscaler provider: 0=Passthrough, 1=NVIDIA DLSS.");

	ConsoleVariable<bool> CVarUpscalerDiagnosticsEnabled(
	    "r.Upscaler.Diagnostics",
	    false,
	    "Enable additional renderer upscaler diagnostics.");
}

UpscalerSettings BuildUpscalerSettingsFromCVars() noexcept
{
	return UpscalerSettings{
	    .RequestedProvider = CVarUpscalerProvider.Get(),
	    .DiagnosticsEnabled = CVarUpscalerDiagnosticsEnabled.Get()};
}
