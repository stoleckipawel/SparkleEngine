#include "PCH.h"
#include "D3D12BindingLayout.h"

#include "D3D12RootSignatureBuilder.h"

#include "D3D12SamplerLibrary.h"

#include "Renderer/Public/ShaderParameters/PassParameterLayout.h"

#include <cassert>

class D3D12BindingLayoutCompilerImpl final
{
  public:
	static std::unique_ptr<D3D12BindingLayout> Compile(D3D12Rhi& rhi, const D3D12BindingLayoutCompileDesc& desc)
	{
		assert(desc.ParameterLayout != nullptr);

		D3D12RootSignatureBuilder builder;
		builder.SetFlags(desc.RootSignatureFlags);

		std::vector<D3D12CompiledBinding> bindings;
		bindings.reserve(desc.ParameterLayout->GetParameterCount());

		std::uint32_t cbvRegister = 0;
		std::uint32_t srvRegister = 0;
		std::uint32_t uavRegister = 0;
		std::uint32_t samplerRegister = 0;

		for (const PassParameterDesc& parameter : desc.ParameterLayout->GetParameters())
		{
			const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(parameter.Visibility);
			switch (parameter.Kind)
			{
				case ShaderParameterSemanticKind::UniformData:
					CompileUniformBinding(builder, bindings, parameter, desc, cbvRegister, visibility);
					break;
				case ShaderParameterSemanticKind::ReadTexture:
				case ShaderParameterSemanticKind::ReadBuffer:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    parameter,
					    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					    D3D12CompiledBindingType::DescriptorTableShaderResourceView,
					    srvRegister,
					    visibility);
					break;
				case ShaderParameterSemanticKind::RWTexture:
				case ShaderParameterSemanticKind::RWBuffer:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    parameter,
					    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
					    D3D12CompiledBindingType::DescriptorTableUnorderedAccessView,
					    uavRegister,
					    visibility);
					break;
				case ShaderParameterSemanticKind::SamplerSet:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    parameter,
					    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
					    D3D12CompiledBindingType::DescriptorTableSampler,
					    samplerRegister,
					    visibility);
					break;
				case ShaderParameterSemanticKind::RenderTarget:
				case ShaderParameterSemanticKind::DepthTarget:
					break;
				default:
					assert(false);
					break;
			}
		}

		return std::make_unique<D3D12BindingLayout>(*desc.ParameterLayout, builder.Build(rhi, desc.DebugName), std::move(bindings));
	}

  private:
	static void CompileUniformBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<D3D12CompiledBinding>& bindings,
	    const PassParameterDesc& parameter,
	    const D3D12BindingLayoutCompileDesc& desc,
	    std::uint32_t& cbvRegister,
	    D3D12_SHADER_VISIBILITY visibility)
	{
		assert(parameter.ValueSizeInBytes > 0);

		if (desc.InlineUniformDataAsRootConstants)
		{
			assert((parameter.ValueSizeInBytes % sizeof(std::uint32_t)) == 0);
			const std::uint32_t rootParameterIndex = builder.AddRootConstants(
			    parameter.ValueSizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t)),
			    cbvRegister,
			    0,
			    visibility);
			bindings.push_back(
			    D3D12CompiledBinding{
			        .Name = parameter.Name,
			        .Type = D3D12CompiledBindingType::RootConstants,
			        .RootParameterIndex = rootParameterIndex,
			        .ShaderRegister = cbvRegister,
			        .RegisterSpace = 0,
			        .DescriptorCount = parameter.ValueSizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t)),
			        .Visibility = visibility});
			++cbvRegister;
			return;
		}

		const std::uint32_t rootParameterIndex = builder.AddConstantBufferView(cbvRegister, 0, visibility);
		bindings.push_back(
		    D3D12CompiledBinding{
		        .Name = parameter.Name,
		        .Type = D3D12CompiledBindingType::RootConstantBufferView,
		        .RootParameterIndex = rootParameterIndex,
		        .ShaderRegister = cbvRegister,
		        .RegisterSpace = 0,
		        .DescriptorCount = 1,
		        .Visibility = visibility});
		++cbvRegister;
	}

	static void CompileDescriptorTableBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<D3D12CompiledBinding>& bindings,
	    const PassParameterDesc& parameter,
	    D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
	    D3D12CompiledBindingType bindingType,
	    std::uint32_t& nextShaderRegister,
	    D3D12_SHADER_VISIBILITY visibility)
	{
		const std::uint32_t rootParameterIndex =
		    builder.AddDescriptorTable(rangeType, GetDescriptorCount(parameter, rangeType), nextShaderRegister, visibility);
		bindings.push_back(
		    D3D12CompiledBinding{
		        .Name = parameter.Name,
		        .Type = bindingType,
		        .RootParameterIndex = rootParameterIndex,
		        .ShaderRegister = nextShaderRegister,
		        .RegisterSpace = 0,
		        .DescriptorCount = GetDescriptorCount(parameter, rangeType),
		        .Visibility = visibility});
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
    std::vector<D3D12CompiledBinding> bindings) noexcept :
    m_parameterLayout(&parameterLayout), m_rootSignature(std::move(rootSignature)), m_bindings(std::move(bindings))
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

const std::vector<D3D12CompiledBinding>& D3D12BindingLayout::GetBindings() const noexcept
{
	return m_bindings;
}

const D3D12CompiledBinding* D3D12BindingLayout::FindBinding(const char* name) const noexcept
{
	if (name == nullptr)
	{
		return nullptr;
	}

	for (const D3D12CompiledBinding& binding : m_bindings)
	{
		if (binding.Name == name)
		{
			return &binding;
		}
	}

	return nullptr;
}

std::unique_ptr<D3D12BindingLayout> D3D12BindingLayoutCompiler::Compile(D3D12Rhi& rhi, const D3D12BindingLayoutCompileDesc& desc)
{
	return D3D12BindingLayoutCompilerImpl::Compile(rhi, desc);
}