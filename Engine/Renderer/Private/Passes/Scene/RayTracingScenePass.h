#pragma once

struct RenderFrame;

class FrameGraphBuilder;
class RenderRayTracingScene;
struct RenderFrameGraphResources;

void AddRayTracingScenePass(FrameGraphBuilder& builder, const RenderFrame& frame, RenderRayTracingScene& rayTracingScene, RenderFrameGraphResources& resources);
