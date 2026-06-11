#pragma once

#include "Upscaling/UpscalerProvider.h"

struct UpscalerSettings final
{
	EUpscalerProviderKind RequestedProvider = EUpscalerProviderKind::Passthrough;
	bool DiagnosticsEnabled = false;
};

UpscalerSettings BuildUpscalerSettingsFromCVars() noexcept;
