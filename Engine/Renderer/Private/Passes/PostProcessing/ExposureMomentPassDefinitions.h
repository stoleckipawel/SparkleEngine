#pragma once

#include "Passes/PostProcessing/ExposureMomentResources.h"

class FrameGraphBuilder;

void AddExposureSceneReductionPass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneColor,
    const ExposureMomentTexture& output);
void AddExposureTextureReductionPass(
    FrameGraphBuilder& builder,
    const ExposureMomentTexture& input,
    const ExposureMomentTexture& output);
void AddExposureSceneDownsamplePass(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle sceneColor,
    const ExposureMomentTexture& output);
void AddExposureTextureDownsamplePass(
    FrameGraphBuilder& builder,
    const ExposureMomentTexture& input,
    const ExposureMomentTexture& output);
