#pragma once

#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddPresentationPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources);
