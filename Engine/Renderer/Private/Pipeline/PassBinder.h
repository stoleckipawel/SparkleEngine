#pragma once

#include "Pipeline/PassBindingOverrides.h"

#include <cstdint>
#include <span>

class RenderCommandContext;
class FrameGraph;
struct PassParameterBinding;
class PassParameterSet;
class RenderHardwareInterface;

class PassBinder final
{
  public:
	static void BindGraphics(
	    RenderCommandContext& cmd,
	    const FrameGraph& frameGraph,
	    RenderHardwareInterface* renderHardwareInterface,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr);

	static void BindCompute(
	    RenderCommandContext& cmd,
	    const FrameGraph& frameGraph,
	    RenderHardwareInterface* renderHardwareInterface,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr);

  private:
	static void BindImpl(
	    RenderCommandContext& cmd,
	    const FrameGraph& frameGraph,
	    RenderHardwareInterface* renderHardwareInterface,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames,
	    const PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindCompiledBinding(
	    RenderCommandContext& cmd,
	    const FrameGraph& frameGraph,
	    RenderHardwareInterface* renderHardwareInterface,
	    const CompiledBinding& compiledBinding,
	    const PassParameterBinding* parameterBinding,
	    const PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindRootGpuAddress(
	    RenderCommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    RhiGpuVirtualAddress gpuAddress,
	    bool isCompute);
	static bool TryBindDescriptorTableOverride(
	    RenderCommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    const PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindDescriptorTable(
	    RenderCommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    RhiGpuDescriptorHandle descriptorTable,
	    bool isCompute);
	static void BindDescriptorTable(
	    RenderCommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    RhiDescriptorTableBinding descriptorTable,
	    bool isCompute);
	static void BindRootConstants(
	    RenderCommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    const void* data,
	    std::uint32_t constantCount,
	    bool isCompute);
};