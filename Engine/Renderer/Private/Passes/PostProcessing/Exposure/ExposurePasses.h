#pragma once

struct RenderFrame;

#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddExposurePasses(FrameGraphBuilder& builder, const RenderFrame& frame, const RenderFrameGraphSettings& settings, const RenderFrameGraphResources& resources);
