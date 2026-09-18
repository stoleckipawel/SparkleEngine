#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

struct ViewportCaptureCompletion final
{
	ViewportCaptureId Id;
	ViewportCaptureReadback Readback;
};
