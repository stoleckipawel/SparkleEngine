#pragma once

#include "Diagnostics/RhiDiagnostics.h"

#include <memory>

std::unique_ptr<RenderDiagnostics> CreateRhiDiagnosticsComposition(
    std::unique_ptr<RenderObjectDiagnostics> objectDiagnostics,
    std::unique_ptr<RenderTimingDiagnostics> timingDiagnostics,
    std::unique_ptr<RenderMessageDiagnostics> messageDiagnostics,
    std::unique_ptr<RenderFailureDiagnostics> failureDiagnostics,
    std::unique_ptr<RenderMemoryDiagnostics> memoryDiagnostics,
    bool supportsGpuEvents);
