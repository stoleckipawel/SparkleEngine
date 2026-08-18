#pragma once

#include "GameFramework/Public/Rendering/RenderViewCameraData.h"

struct RenderViewInput final
{
	RenderViewCameraData Camera;
	bool CameraCut = false;
	bool CameraTeleported = false;
};
