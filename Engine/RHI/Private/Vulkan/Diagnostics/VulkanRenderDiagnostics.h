#pragma once

#include <memory>

class RenderDiagnostics;
class VulkanRhi;

std::unique_ptr<RenderDiagnostics> CreateVulkanRenderDiagnostics(VulkanRhi& rhi);