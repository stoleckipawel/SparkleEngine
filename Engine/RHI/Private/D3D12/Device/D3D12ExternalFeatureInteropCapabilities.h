#pragma once

#include "Core/RhiCapabilities.h"

class D3D12Rhi;

RhiAdapterIdentity BuildD3D12AdapterIdentity(const D3D12Rhi* rhi) noexcept;

RhiExternalFeatureInteropCapabilities BuildD3D12ExternalFeatureInteropCapabilities(
    const D3D12Rhi* rhi,
    bool hasGraphicsCommandList) noexcept;
