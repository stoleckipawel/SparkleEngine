#pragma once

#include "Frame/Core/Frame.h"

class FrameGraphBuilder;

void AddExposurePass(FrameGraphBuilder& builder, const FrameBuildSettings& settings, const FrameAssemblyResourceLayout& resources);
