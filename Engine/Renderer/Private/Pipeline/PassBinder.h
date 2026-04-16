#pragma once

#include "Pipeline/PassBindingOverrides.h"

#include <cstdint>
#include <span>

class CommandContext;
class FrameGraph;
struct PassParameterBinding;
class PassParameterSet;

class PassBinder final
{
  public:
	static void BindGraphics(
	    CommandContext& cmd,
	    const FrameGraph& frameGraph,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr);

	static void BindCompute(
	    CommandContext& cmd,
	    const FrameGraph& frameGraph,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr);

  private:
	static void BindImpl(
	    CommandContext& cmd,
	    const FrameGraph& frameGraph,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames,
	    const PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindCompiledBinding(
	    CommandContext& cmd,
	    const FrameGraph& frameGraph,
	    const CompiledBinding& compiledBinding,
	    const PassParameterBinding* parameterBinding,
	    const PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindRootGpuAddress(
	    CommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    RhiGpuVirtualAddress gpuAddress,
	    bool isCompute);
	static void BindDescriptorTable(
	    CommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    RhiGpuDescriptorHandle descriptorTable,
	    bool isCompute);
	static void BindDescriptorTable(
	    CommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    RhiDescriptorTableHandle descriptorTable,
	    bool isCompute);
	static void BindRootConstants(
	    CommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    const void* data,
	    std::uint32_t constantCount,
	    bool isCompute);
};