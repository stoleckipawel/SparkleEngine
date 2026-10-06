#pragma once

struct RenderFrame;

class FrameGraphBuilder;
class ReferencePathTracerSession;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void AddReferencePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    const RenderFrameGraphSettings& settings,
    ReferencePathTracerSession& session,
    RenderFrameGraphResources& resources);
