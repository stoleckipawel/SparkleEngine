#pragma once

#include "Pipeline/PassBindingOverrides.h"
#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

#include <cstdint>
#include <span>
#include <string_view>

class RenderCommandContext;
class FrameGraphResourceCommands;
struct PassParameterBinding;
class PassParameterSet;

class PassBinder final
{
public:
	static void BindGraphics(
	    RenderCommandContext& commandContext,
	    const FrameGraphResourceCommands& resources,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr,
	    bool bindLayout = true);

	static void BindCompute(
	    RenderCommandContext& commandContext,
	    const FrameGraphResourceCommands& resources,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr,
	    bool bindLayout = true);

private:
	struct BindingRequest;

	static void BindImpl(
	    RenderCommandContext& commandContext,
	    const FrameGraphResourceCommands& resources,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames,
	    const PassBindingOverrides* overrides,
	    bool bindLayout,
	    bool isCompute);
	static void BindCompiledBinding(
	    RenderCommandContext& commandContext,
	    const FrameGraphResourceCommands& resources,
	    const CompiledBinding& compiledBinding,
	    const PassParameterBinding* parameterBinding,
	    const PassBindingOverrides* overrides,
	    bool isCompute);
	static void BindConstantBuffer(const BindingRequest& request);
	static void BindReadOnlyAddress(const BindingRequest& request);
	static void BindReadWriteAddress(const BindingRequest& request);
	static void BindAccelerationStructure(const BindingRequest& request);
	static void BindResourceTable(const BindingRequest& request, bool readWrite);
	static void BindSamplerTable(const BindingRequest& request);
	static void BindPushConstantData(const BindingRequest& request);
	static void BindGpuAddress(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    RhiGpuVirtualAddress gpuAddress,
	    bool isCompute);
	static void BindDescriptorTableOverride(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    const PassBindingOverride& bindingOverride,
	    bool isCompute);
	static void BindDescriptorTable(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    RhiGpuDescriptorHandle descriptorTable,
	    bool isCompute);
	static void BindDescriptorTable(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    RhiDescriptorTableBinding descriptorTable,
	    bool isCompute);
	static void BindPushConstants(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    const void* data,
	    std::uint32_t constantCount,
	    bool isCompute);
	static void Require(bool condition, std::string_view message);
};
