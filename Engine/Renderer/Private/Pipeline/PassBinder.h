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
	static void BindRayTracing(
	    RenderCommandContext& commandContext,
	    const FrameGraphResourceCommands& resources,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames = {},
	    const PassBindingOverrides* overrides = nullptr,
	    bool bindLayout = true);

private:
	enum class BindingDomain : std::uint8_t
	{
		Graphics,
		Compute,
		RayTracing,
	};
	struct BindingRequest;

	static void BindImpl(
	    RenderCommandContext& commandContext,
	    const FrameGraphResourceCommands& resources,
	    const RenderBindingLayout& layout,
	    const PassParameterSet& parameterSet,
	    std::span<const char* const> bindingNames,
	    const PassBindingOverrides* overrides,
	    bool bindLayout,
	    BindingDomain domain);
	static void BindCompiledBinding(
	    RenderCommandContext& commandContext,
	    const FrameGraphResourceCommands& resources,
	    const CompiledBinding& compiledBinding,
	    const PassParameterBinding* parameterBinding,
	    const PassBindingOverrides* overrides,
	    BindingDomain domain);
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
	    BindingDomain domain);
	static void BindDescriptorTableOverride(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    const PassBindingOverride& bindingOverride,
	    BindingDomain domain);
	static void BindDescriptorTable(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    RhiGpuDescriptorHandle descriptorTable,
	    BindingDomain domain);
	static void BindDescriptorTable(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    RhiDescriptorTableBinding descriptorTable,
	    BindingDomain domain);
	static void BindPushConstants(
	    RenderCommandContext& commandContext,
	    const CompiledBinding& compiledBinding,
	    const void* data,
	    std::uint32_t constantCount,
	    BindingDomain domain);
	static void Require(bool condition, std::string_view message);
};
