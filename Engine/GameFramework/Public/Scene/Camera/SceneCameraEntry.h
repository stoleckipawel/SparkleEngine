#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Camera/CameraDesc.h"

#include <string>

struct SPARKLE_ENGINE_API SceneCameraEntry
{
	std::string name;
	CameraDesc desc;

	bool IsPerspective() const noexcept { return desc.projectionKind == CameraProjectionKind::Perspective; }
};
