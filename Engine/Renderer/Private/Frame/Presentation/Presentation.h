#pragma once

#include "Frame/Core/Frame.h"

class FrameGraphBuilder;

void AddPresentationPasses(FrameGraphBuilder& builder, const FrameBuildSettings& settings, FrameAssemblyResourceLayout& resources);
