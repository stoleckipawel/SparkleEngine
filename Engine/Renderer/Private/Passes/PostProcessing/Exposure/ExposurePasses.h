#pragma once

#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddExposurePasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, const RenderFrameGraphResources& resources);
