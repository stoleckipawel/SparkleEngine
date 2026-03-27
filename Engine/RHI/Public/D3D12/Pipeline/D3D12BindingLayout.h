#pragma once

#include "../../RHIAPI.h"
#include "D3D12RootSignature.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class D3D12Rhi;
class PassParameterLayout;

enum class D3D12CompiledBindingType : std::uint8_t
{
	RootConstantBufferView,
	RootShaderResourceView,
	RootUnorderedAccessView,
	DescriptorTableShaderResourceView,
	DescriptorTableUnorderedAccessView,
	DescriptorTableSampler,
	RootConstants,
};

struct D3D12CompiledBinding
{
	std::string Name;
	D3D12CompiledBindingType Type = D3D12CompiledBindingType::DescriptorTableShaderResourceView;
	std::uint32_t RootParameterIndex = 0;
	std::uint32_t ShaderRegister = 0;
	std::uint32_t RegisterSpace = 0;
	std::uint32_t DescriptorCount = 0;
	D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL;
};

struct D3D12BindingLayoutCompileDesc
{
	const PassParameterLayout* ParameterLayout = nullptr;
	D3D12_ROOT_SIGNATURE_FLAGS RootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	const wchar_t* DebugName = L"RHI_BindingLayout";
	bool InlineUniformDataAsRootConstants = false;
};

class SPARKLE_RHI_API D3D12BindingLayout final
{
  public:
	D3D12BindingLayout(
	    const PassParameterLayout& parameterLayout,
	    std::unique_ptr<D3D12RootSignature> rootSignature,
	    std::vector<D3D12CompiledBinding> bindings) noexcept;
	~D3D12BindingLayout() noexcept;

	D3D12BindingLayout(const D3D12BindingLayout&) = delete;
	D3D12BindingLayout& operator=(const D3D12BindingLayout&) = delete;
	D3D12BindingLayout(D3D12BindingLayout&&) = delete;
	D3D12BindingLayout& operator=(D3D12BindingLayout&&) = delete;

	D3D12RootSignature& GetRootSignature() const noexcept;
	const PassParameterLayout& GetParameterLayout() const noexcept;
	const std::vector<D3D12CompiledBinding>& GetBindings() const noexcept;
	const D3D12CompiledBinding* FindBinding(const char* name) const noexcept;

  private:
	const PassParameterLayout* m_parameterLayout = nullptr;
	std::unique_ptr<D3D12RootSignature> m_rootSignature;
	std::vector<D3D12CompiledBinding> m_bindings;
};

class SPARKLE_RHI_API D3D12BindingLayoutCompiler final
{
  public:
	static std::unique_ptr<D3D12BindingLayout> Compile(D3D12Rhi& rhi, const D3D12BindingLayoutCompileDesc& desc);
};