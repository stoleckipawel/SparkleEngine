#pragma once

#include "Passes/PostProcessing/Exposure/ExposureMomentResources.h"

class FrameGraphBuilder;

void AddExposureSceneDownsamplePass(FrameGraphBuilder& builder, FrameGraphTextureHandle sceneColor, const ExposureMomentTexture& output);
void AddExposureTextureDownsamplePass(FrameGraphBuilder& builder, const ExposureMomentTexture& input, const ExposureMomentTexture& output);
