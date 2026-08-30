#pragma once

#include "Device/RenderHardwareInterface.h"
#include "D3D12RootSignature.h"

#include <memory>
#include <string>
#include <vector>

class D3D12Rhi;
class PassParameterLayout;

class D3D12BindingLayout final : public RenderBindingLayout
{
public:
	D3D12BindingLayout(
	    const PassParameterLayout& parameterLayout,
	    std::unique_ptr<D3D12RootSignature> rootSignature,
	    std::vector<CompiledBinding> bindings,
	    std::vector<std::string> bindingNames) noexcept;
	~D3D12BindingLayout() noexcept;

	D3D12BindingLayout(const D3D12BindingLayout&) = delete;
	D3D12BindingLayout& operator=(const D3D12BindingLayout&) = delete;
	D3D12BindingLayout(D3D12BindingLayout&&) = delete;
	D3D12BindingLayout& operator=(D3D12BindingLayout&&) = delete;

	D3D12RootSignature& GetRootSignature() const noexcept;

private:
	std::unique_ptr<D3D12RootSignature> m_rootSignature;
};

class D3D12BindingLayoutCompiler final
{
public:
	static std::unique_ptr<D3D12BindingLayout> Compile(D3D12Rhi& rhi, const RenderBindingLayoutCompileDesc& desc);
};
