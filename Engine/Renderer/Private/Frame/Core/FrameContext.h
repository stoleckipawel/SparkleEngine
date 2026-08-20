#pragma once

#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

struct FrameContext
{
	PreparedRenderScene preparedScene = {};
	RenderView view = {};
};
