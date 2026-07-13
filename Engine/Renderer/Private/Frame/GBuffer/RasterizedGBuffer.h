#pragma once

#include "Frame/Targets/FrameRenderTargets.h"

class FrameGraphBuilder;
struct FrameAssemblyExternalResources;

void AddRasterizedGBufferPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& targets,
    const FrameAssemblyExternalResources& externalResources);
