#pragma once

#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraphBuilder;

void AddPresentationPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources);
