#pragma once

#include "Core/Public/Console/CVar.h"

extern ConsoleVariable<bool> CVarSkyEnabled;

struct RenderFrame;

#include "Renderer/Public/Viewport/ViewportContracts.h"

class FrameGraphBuilder;
struct RenderFrameGraphResources;

void AddSkyPass(FrameGraphBuilder& builder, const RenderFrame& frame, RenderViewportExtent sceneExtent, const RenderFrameGraphResources& resources);
