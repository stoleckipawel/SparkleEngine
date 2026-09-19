#pragma once

class FrameGraphBuilder;
class ReferencePathTracerSession;
struct RenderFrameGraphResources;
struct RenderFrameGraphSettings;

void AddReferencePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    ReferencePathTracerSession& session,
    RenderFrameGraphResources& resources);
