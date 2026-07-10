#pragma once

#include "Frame/Core/FrameAssembly.h"
#include "Frame/Lighting/RestirIndirectReservoirs.h"

class FrameGraphBuilder;

void AddRestirIndirectTemporalPass(
    FrameGraphBuilder& builder,
    const RestirIndirectWorkingReservoirs& workingReservoirs,
    const FrameAssemblyResourceLayout& resources);
