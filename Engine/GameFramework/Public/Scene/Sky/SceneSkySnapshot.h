#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Sky/SceneSkyDesc.h"

#include <optional>

struct SPARKLE_ENGINE_API SceneSkySnapshot
{
	std::optional<SceneSkyDesc> sky;

	bool HasSky() const noexcept { return sky.has_value(); }
};
