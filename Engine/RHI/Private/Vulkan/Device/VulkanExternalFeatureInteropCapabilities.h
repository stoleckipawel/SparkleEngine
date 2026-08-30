#pragma once

#include "Core/RhiCapabilities.h"

class VulkanRhi;

RhiExternalFeatureInteropCapabilities BuildVulkanExternalFeatureInteropCapabilities(
    const VulkanRhi* rhi,
    bool hasGraphicsCommandContext) noexcept;
