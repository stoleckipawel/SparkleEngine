#pragma once

#include "FrameGraphProducts.h"

#include <cstdint>

class FrameGraph;

namespace FrameGraphFeatures
{
FrameGraphComputeShowcaseOutputs AddComputeClearShowcasePass(
    FrameGraph& frameGraph,
    std::uint32_t width,
    std::uint32_t height);
}
