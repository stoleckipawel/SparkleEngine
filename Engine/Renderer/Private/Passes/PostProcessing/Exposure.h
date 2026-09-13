#pragma once

#include "Frame/Graph/RenderFrameGraphSettings.h"

class FrameGraphBuilder;

void AddExposurePass(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, const RenderFrameGraphResources& resources);
