#pragma once

#include "Core/RhiCapabilities.h"

class D3D12Rhi;

RhiExternalFeatureInteropCapabilities BuildD3D12ExternalFeatureInteropCapabilities(
    const D3D12Rhi* rhi,
    bool hasGraphicsCommandList) noexcept;

