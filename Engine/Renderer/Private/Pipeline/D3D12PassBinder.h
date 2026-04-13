#pragma once

#include "RHI/Public/D3D12/Pipeline/D3D12PassBindingOverrides.h"

#include <cstdint>
#include <d3d12.h>
#include <span>

class CommandContext;
class D3D12BindingLayout;
struct D3D12CompiledBinding;
class FrameGraph;
struct PassParameterBinding;
class PassParameterSet;

class D3D12PassBinder final
{
  public:
	static void BindGraphics(
	    CommandContext& cmd,
	    const FrameGraph& frameGraph,
	    const D3D12BindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const D3D12PassBindingOverrides* overrides = nullptr);

	static void BindCompute(
	    CommandContext& cmd,
	    const FrameGraph& frameGraph,
	    const D3D12BindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const D3D12PassBindingOverrides* overrides = nullptr);

  private:
	static void BindImpl(
	    CommandContext& cmd,
	    const FrameGraph& frameGraph,
	    const D3D12BindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames,
	    const D3D12PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindCompiledBinding(
	    CommandContext& cmd,
	    const FrameGraph& frameGraph,
	    const D3D12CompiledBinding& compiledBinding,
	    const PassParameterBinding* parameterBinding,
	    const D3D12PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindRootGpuAddress(
	    CommandContext& cmd,
	    const D3D12CompiledBinding& compiledBinding,
	    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
	    bool isCompute);
	static void BindDescriptorTable(
	    CommandContext& cmd,
	    const D3D12CompiledBinding& compiledBinding,
	    D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable,
	    bool isCompute);
	static void BindRootConstants(
	    CommandContext& cmd,
	    const D3D12CompiledBinding& compiledBinding,
	    const void* data,
	    std::uint32_t constantCount,
	    bool isCompute);
};