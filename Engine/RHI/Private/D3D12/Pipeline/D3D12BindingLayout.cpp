#include "PCH.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"

#include "D3D12/Pipeline/D3D12RootSignatureBuilder.h"

#include "D3D12/Samplers/D3D12SamplerLibrary.h"

#include "ShaderParameters/PassParameterLayout.h"

#include <cassert>
#include <string_view>

class D3D12BindingLayoutCompilerImpl final
{
  public:
	static std::unique_ptr<D3D12BindingLayout> Compile(D3D12Rhi& rhi, const RenderBindingLayoutCompileDesc& desc)
	{
		assert(desc.ParameterLayout != nullptr);

		D3D12RootSignatureBuilder builder;
		builder.SetFlags(desc.AllowInputAssemblerInputLayout ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		                                                    : D3D12_ROOT_SIGNATURE_FLAG_NONE);

		std::vector<CompiledBinding> bindings;
		std::vector<std::string> bindingNames;
		bindings.reserve(desc.ParameterLayout->GetParameterCount());
		bindingNames.reserve(desc.ParameterLayout->GetParameterCount());

		std::uint32_t cbvRegister = 0;
		std::uint32_t srvRegister = 0;
		std::uint32_t uavRegister = 0;
		std::uint32_t samplerRegister = 0;

		for (const PassParameterDesc& parameter : desc.ParameterLayout->GetParameters())
		{
			switch (parameter.Kind)
			{
				case ShaderParameterSemanticKind::UniformData:
					CompileUniformBinding(builder, bindings, bindingNames, parameter, desc, cbvRegister);
					break;
				case ShaderParameterSemanticKind::ReadTexture:
				case ShaderParameterSemanticKind::ReadBuffer:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    parameter,
					    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					    CompiledBindingType::DescriptorTableShaderResourceView,
					    srvRegister);
					break;
				case ShaderParameterSemanticKind::RWTexture:
				case ShaderParameterSemanticKind::RWBuffer:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    parameter,
					    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
					    CompiledBindingType::DescriptorTableUnorderedAccessView,
					    uavRegister);
					break;
				case ShaderParameterSemanticKind::SamplerSet:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    parameter,
					    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
					    CompiledBindingType::DescriptorTableSampler,
					    samplerRegister);
					break;
				case ShaderParameterSemanticKind::RenderTarget:
				case ShaderParameterSemanticKind::DepthTarget:
					break;
				default:
					assert(false);
					break;
			}
		}

		return std::make_unique<D3D12BindingLayout>(
		    *desc.ParameterLayout,
		    builder.Build(rhi, desc.DebugName),
		    std::move(bindings),
		    std::move(bindingNames));
	}

  private:
	static void CompileUniformBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<CompiledBinding>& bindings,
	    std::vector<std::string>& bindingNames,
	    const PassParameterDesc& parameter,
	    const RenderBindingLayoutCompileDesc& desc,
	    std::uint32_t& cbvRegister)
	{
		assert(parameter.ValueSizeInBytes > 0);
		const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(parameter.Visibility);
		bindingNames.push_back(parameter.Name);

		if (desc.InlineUniformDataAsRootConstants)
		{
			assert((parameter.ValueSizeInBytes % sizeof(std::uint32_t)) == 0);
			const std::uint32_t rootParameterIndex = builder.AddRootConstants(
			    parameter.ValueSizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t)),
			    cbvRegister,
			    0,
			    visibility);
			bindings.push_back(
			    CompiledBinding{
			        .Name = bindingNames.back().c_str(),
			        .Type = CompiledBindingType::RootConstants,
			        .RootParameterIndex = rootParameterIndex,
			        .ShaderRegister = cbvRegister,
			        .RegisterSpace = 0,
			        .DescriptorCount = parameter.ValueSizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t))});
			++cbvRegister;
			return;
		}

		const std::uint32_t rootParameterIndex = builder.AddConstantBufferView(cbvRegister, 0, visibility);
		bindings.push_back(
		    CompiledBinding{
		        .Name = bindingNames.back().c_str(),
		        .Type = CompiledBindingType::RootConstantBufferView,
		        .RootParameterIndex = rootParameterIndex,
		        .ShaderRegister = cbvRegister,
		        .RegisterSpace = 0,
		        .DescriptorCount = 1});
		++cbvRegister;
	}

	static void CompileDescriptorTableBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<CompiledBinding>& bindings,
	    std::vector<std::string>& bindingNames,
	    const PassParameterDesc& parameter,
	    D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
	    CompiledBindingType bindingType,
	    std::uint32_t& nextShaderRegister)
	{
		const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(parameter.Visibility);
		bindingNames.push_back(parameter.Name);
		const std::uint32_t rootParameterIndex =
		    builder.AddDescriptorTable(rangeType, GetDescriptorCount(parameter, rangeType), nextShaderRegister, visibility);
		bindings.push_back(
		    CompiledBinding{
		        .Name = bindingNames.back().c_str(),
		        .Type = bindingType,
		        .RootParameterIndex = rootParameterIndex,
		        .ShaderRegister = nextShaderRegister,
		        .RegisterSpace = 0,
		        .DescriptorCount = GetDescriptorCount(parameter, rangeType)});
		nextShaderRegister += GetDescriptorCount(parameter, rangeType);
	}

	static std::uint32_t GetDescriptorCount(const PassParameterDesc& parameter, D3D12_DESCRIPTOR_RANGE_TYPE rangeType) noexcept
	{
		if (rangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER && parameter.Kind == ShaderParameterSemanticKind::SamplerSet)
		{
			return D3D12SamplerLibrary::GetSamplerCount();
		}

		return parameter.ArrayCount;
	}

	static D3D12_SHADER_VISIBILITY ToD3D12Visibility(ShaderStageVisibility visibility) noexcept
	{
		if (visibility == ShaderStageVisibility::Vertex)
		{
			return D3D12_SHADER_VISIBILITY_VERTEX;
		}

		if (visibility == ShaderStageVisibility::Pixel)
		{
			return D3D12_SHADER_VISIBILITY_PIXEL;
		}

		return D3D12_SHADER_VISIBILITY_ALL;
	}
};

D3D12BindingLayout::D3D12BindingLayout(
    const PassParameterLayout& parameterLayout,
    std::unique_ptr<D3D12RootSignature> rootSignature,
	std::vector<CompiledBinding> bindings,
	std::vector<std::string> bindingNames) noexcept :
	m_parameterLayout(&parameterLayout),
	m_rootSignature(std::move(rootSignature)),
	m_bindings(std::move(bindings)),
	m_bindingNames(std::move(bindingNames))
{
	assert(m_parameterLayout != nullptr);
	assert(m_rootSignature != nullptr);
}

D3D12BindingLayout::~D3D12BindingLayout() noexcept = default;

D3D12RootSignature& D3D12BindingLayout::GetRootSignature() const noexcept
{
	assert(m_rootSignature != nullptr);
	return *m_rootSignature;
}

const PassParameterLayout& D3D12BindingLayout::GetParameterLayout() const noexcept
{
	assert(m_parameterLayout != nullptr);
	return *m_parameterLayout;
}

const CompiledBinding* D3D12BindingLayout::GetBindings() const noexcept
{
	return m_bindings.data();
}

std::size_t D3D12BindingLayout::GetBindingCount() const noexcept
{
	return m_bindings.size();
}

const CompiledBinding* D3D12BindingLayout::FindBinding(const char* name) const noexcept
{
	if (name == nullptr)
	{
		return nullptr;
	}

	for (const CompiledBinding& binding : m_bindings)
	{
		if (binding.Name != nullptr && std::string_view(binding.Name) == name)
		{
			return &binding;
		}
	}

	return nullptr;
}

std::unique_ptr<D3D12BindingLayout> D3D12BindingLayoutCompiler::Compile(D3D12Rhi& rhi, const RenderBindingLayoutCompileDesc& desc)
{
	return D3D12BindingLayoutCompilerImpl::Compile(rhi, desc);
}