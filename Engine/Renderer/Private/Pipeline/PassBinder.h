#pragma once

#include "Pipeline/PassBindingOverrides.h"

#include <cstdint>
#include <span>

class RenderCommandContext;
class FrameGraphResourceCommands;
struct PassParameterBinding;
class PassParameterSet;
class RenderHardwareInterface;

class PassBinder final
{
  public:
	static void BindGraphics(
	    RenderCommandContext& cmd,
	    const FrameGraphResourceCommands& resources,
	    RenderHardwareInterface* renderHardwareInterface,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr);

	static void BindCompute(
	    RenderCommandContext& cmd,
	    const FrameGraphResourceCommands& resources,
	    RenderHardwareInterface* renderHardwareInterface,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr);

  private:
	static void BindImpl(
	    RenderCommandContext& cmd,
	    const FrameGraphResourceCommands& resources,
	    RenderHardwareInterface* renderHardwareInterface,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames,
	    const PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindCompiledBinding(
	    RenderCommandContext& cmd,
	    const FrameGraphResourceCommands& resources,
	    RenderHardwareInterface* renderHardwareInterface,
	    const CompiledBinding& compiledBinding,
	    const PassParameterBinding* parameterBinding,
	    const PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindGpuAddress(
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
	static void BindPushConstants(
	    RenderCommandContext& cmd,
	    const CompiledBinding& compiledBinding,
	    const void* data,
	    std::uint32_t constantCount,
	    bool isCompute);
};