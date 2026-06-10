#pragma once

#include "Core/Public/Console/CVar.h"

struct DLAASettings
{
	bool Enabled = false;
	float BlendFactor = 0.15f;
};

DLAASettings BuildDLAASettingsFromCVars() noexcept;

