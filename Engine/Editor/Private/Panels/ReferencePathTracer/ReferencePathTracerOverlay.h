#pragma once

#include "Panels/ViewportOutputAction.h"

struct ViewportRenderProgress;
struct ViewportRenderRequest;

void DrawReferencePathTracerOverlay(
    const ViewportRenderProgress& progress,
    ViewportRenderRequest& request,
    ViewportOutputAction& outputAction) noexcept;
