#pragma once

#include "../../RHIAPI.h"

#include <cstdint>
#include <d3d12.h>
#include <span>
#include <string>
#include <vector>

class CommandContext;
class D3D12BindingLayout;
struct D3D12CompiledBinding;
class FrameGraph;
struct PassParameterBinding;
class PassParameterSet;

enum class D3D12BindingOverrideType : std::uint8_t
{
	ConstantBufferView,
	ShaderResourceView,
	UnorderedAccessView,
	DescriptorTable,
	RootConstants,
};

struct D3D12BindingOverride
{
	std::string Name;
	D3D12BindingOverrideType Type = D3D12BindingOverrideType::DescriptorTable;
	D3D12_GPU_VIRTUAL_ADDRESS GpuAddress = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTable = {};
	const void* ConstantsData = nullptr;
	std::uint32_t ConstantCount = 0;
};

class SPARKLE_RHI_API D3D12PassBindingOverrides final
{
  public:
	void SetConstantBufferView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
	void SetShaderResourceView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
	void SetUnorderedAccessView(const char* name, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
	void SetDescriptorTable(const char* name, D3D12_GPU_DESCRIPTOR_HANDLE descriptorTable);
	void SetRootConstants(const char* name, const void* data, std::uint32_t constantCount);

	const D3D12BindingOverride* Find(const char* name, D3D12BindingOverrideType type) const noexcept;

  private:
	std::vector<D3D12BindingOverride> m_overrides;
};

class SPARKLE_RHI_API D3D12PassBinder final
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