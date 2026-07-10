#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"

class FrameGraphBuilder;

void AddLinearizeDeviceZPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle deviceZ,
    FrameGraphTextureHandle sceneDepth);
