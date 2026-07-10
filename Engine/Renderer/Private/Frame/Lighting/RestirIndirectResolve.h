#pragma once

#include "Frame/Core/FrameAssembly.h"

class FrameGraphBuilder;

void AddRestirIndirectResolvePass(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources);
