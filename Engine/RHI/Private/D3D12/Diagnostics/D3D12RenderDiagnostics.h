#pragma once

#include <memory>

class D3D12Rhi;
class RenderDiagnostics;

std::unique_ptr<RenderDiagnostics> CreateD3D12RenderDiagnostics(D3D12Rhi& rhi);