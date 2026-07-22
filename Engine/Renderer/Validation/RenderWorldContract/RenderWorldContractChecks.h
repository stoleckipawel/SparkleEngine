#pragma once

#include "GameFramework/Public/Rendering/RenderInputFrame.h"
#include "SceneData/RenderWorld.h"

#include <chrono>
#include <vector>

bool ValidateDeterministicRenderWorldReplay(
    const std::vector<RenderInputFrame>& recording,
    RenderObjectId retainedObject,
    RenderWorld& retainedWorld,
    std::chrono::steady_clock::duration& replayElapsed);
bool ValidateRenderWorldOrdering(const std::vector<RenderInputFrame>& recording);
bool ValidateRejectedRenderWorldDeltas(const RenderInputFrame& resetFrame, RenderObjectId object);
bool ValidateRenderInputFrameAdmission(const std::vector<RenderInputFrame>& recording);
bool ValidateRetainedRenderWorldOwnership(const RenderWorld& world, RenderObjectId retainedObject);
