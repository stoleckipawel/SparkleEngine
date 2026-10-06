#pragma once

#include "Core/Public/Console/CVar.h"

extern ConsoleVariable<bool> CVarDirectShadows;

bool IsDirectShadowsActive() noexcept;
void RequireDirectShadowSignal(bool available) noexcept;
