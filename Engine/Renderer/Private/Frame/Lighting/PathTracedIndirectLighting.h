#pragma once

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;

void AddPathTracedIndirectLightingPass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources);
