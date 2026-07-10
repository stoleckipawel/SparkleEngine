#pragma once

class FrameGraphBuilder;
struct FrameAssemblyResourceLayout;

void AddPathTracedDirectLightingPass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources);
