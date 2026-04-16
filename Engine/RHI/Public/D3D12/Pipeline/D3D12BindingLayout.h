#pragma once

#include "../../RHIAPI.h"
#include "../../Interop/RenderHardwareInterface.h"
#include "D3D12RootSignature.h"

#include <memory>
#include <string>
#include <vector>

class D3D12Rhi;
class PassParameterLayout;

class SPARKLE_RHI_API D3D12BindingLayout final : public RenderBindingLayout
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
	const PassParameterLayout& GetParameterLayout() const noexcept override;
	const CompiledBinding* GetBindings() const noexcept override;
	std::size_t GetBindingCount() const noexcept override;
	const CompiledBinding* FindBinding(const char* name) const noexcept override;

  private:
	const PassParameterLayout* m_parameterLayout = nullptr;
	std::unique_ptr<D3D12RootSignature> m_rootSignature;
	std::vector<CompiledBinding> m_bindings;
	std::vector<std::string> m_bindingNames;
};

class SPARKLE_RHI_API D3D12BindingLayoutCompiler final
{
  public:
	static std::unique_ptr<D3D12BindingLayout> Compile(D3D12Rhi& rhi, const RenderBindingLayoutCompileDesc& desc);
};