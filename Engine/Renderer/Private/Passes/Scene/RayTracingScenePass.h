#pragma once

class FrameGraphBuilder;
class RenderRayTracingScene;
struct RenderFrameGraphResources;

void AddRayTracingScenePass(FrameGraphBuilder& builder, RenderRayTracingScene& rayTracingScene, RenderFrameGraphResources& resources);
