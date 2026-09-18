#pragma once

#include "Panels/ReferencePathTracer/ReferencePathTracerOutput.h"

struct ViewportRenderProgress;
struct ViewportRenderRequest;

void DrawReferencePathTracerOverlay(
    const ViewportRenderProgress& progress,
    ViewportRenderRequest& request,
    ReferencePathTracerOutputAction& outputAction) noexcept;
