#pragma once

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddLightingTargetClearPass(FrameGraphBuilder& builder, const RenderFrameGraphResources& resources);
