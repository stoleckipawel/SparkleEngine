#pragma once

#include "Frame/RenderFrameTime.h"
#include "Renderer/Public/UI/UiRenderPacket.h"
#include "Rendering/RenderFrameSubmission.h"

struct RenderExecutionRequest final
{
	RenderFrameSubmission Submission;
	RenderFrameTime Time;
	UiRenderPacket Ui;
};
