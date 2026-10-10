#pragma once

#include "FrameGraph/FrameGraphBufferHandle.h"
#include "Passes/PostProcessing/Exposure/ExposureMomentResources.h"

class FrameGraphBuilder;
struct RenderViewportExtent;

void AddExposureHistogramClearPass(FrameGraphBuilder& builder, FrameGraphBufferHandle histogram);
void AddExposureHistogramBuildPass(FrameGraphBuilder& builder, FrameGraphTextureHandle sceneColor, RenderViewportExtent extent, FrameGraphBufferHandle histogram);
void AddExposureHistogramResolvePass(FrameGraphBuilder& builder, FrameGraphBufferHandle histogram, const ExposureMomentTexture& output);
