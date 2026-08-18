#pragma once

#include "Frame/Core/Frame.h"

class FrameGraphBuilder;

void CreateFrameSceneResources(FrameGraphBuilder& builder, const FrameBuildSettings& settings, FrameAssemblyResourceLayout& resources);
