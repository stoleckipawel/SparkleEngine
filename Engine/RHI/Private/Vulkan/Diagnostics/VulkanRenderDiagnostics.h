#pragma once

#include <memory>

class RenderDiagnostics;
class VulkanRhi;
class VulkanGpuMemoryAllocator;

std::unique_ptr<RenderDiagnostics> CreateVulkanRenderDiagnostics(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator);