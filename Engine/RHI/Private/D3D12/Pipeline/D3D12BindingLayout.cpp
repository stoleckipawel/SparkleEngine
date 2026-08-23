#include "PCH.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"

#include "D3D12/Pipeline/D3D12RootSignatureBuilder.h"

#include "Pipeline/RhiShaderBindingReflection.h"
#include "ShaderParameters/PassParameterLayout.h"

#include <cassert>
#include <string_view>
#include <vector>

class D3D12BindingLayoutCompilerImpl final
{
  public:
	static std::unique_ptr<D3D12BindingLayout> Compile(D3D12Rhi& rhi, const RenderBindingLayoutCompileDesc& desc)
	{
		assert(desc.ParameterLayout != nullptr);
		assert(!desc.Shaders.empty());
		const std::vector<PassParameterDesc>& bindingRecords = desc.ParameterLayout->GetParameters();

		D3D12RootSignatureBuilder builder;
		builder.SetFlags(
		    desc.AllowInputAssemblerInputLayout ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		                                        : D3D12_ROOT_SIGNATURE_FLAG_NONE);

		std::vector<CompiledBinding> bindings;
		std::vector<std::string> bindingNames;
		bindings.reserve(bindingRecords.size() * static_cast<std::size_t>(ShaderStage::Count));
		bindingNames.reserve(bindingRecords.size() * static_cast<std::size_t>(ShaderStage::Count));

		for (const PassParameterDesc& bindingRecord : bindingRecords)
		{
			const std::string_view bindingName = bindingRecord.Name;
			assert(!bindingName.empty());
			switch (bindingRecord.Kind)
			{
				case ShaderParameterSemanticKind::UniformData:
					CompileUniformBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    desc);
					break;
				case ShaderParameterSemanticKind::ReadTexture:
				case ShaderParameterSemanticKind::ReadBuffer:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    desc.Shaders,
					    *desc.ParameterLayout,
					    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					    CompiledBindingType::ReadOnlyResourceTable);
					break;
				case ShaderParameterSemanticKind::AccelerationStructure:
					CompileRootShaderResourceBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    desc.Shaders,
					    *desc.ParameterLayout);
					break;
				case ShaderParameterSemanticKind::RWTexture:
				case ShaderParameterSemanticKind::RWBuffer:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    desc.Shaders,
					    *desc.ParameterLayout,
					    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
					    CompiledBindingType::ReadWriteResourceTable);
					break;
				case ShaderParameterSemanticKind::SamplerSet:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    desc.Shaders,
					    *desc.ParameterLayout,
					    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
					    CompiledBindingType::SamplerTable);
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
	    const PassParameterDesc& bindingRecord,
	    std::string_view bindingName,
	    const RenderBindingLayoutCompileDesc& desc)
	{
		assert(bindingRecord.ValueSizeInBytes > 0);
		const std::vector<RhiReflectedBindingLocation> reflectedLocations = RhiShaderBindingReflection::ResolveLocations(
		    desc.Shaders,
		    *desc.ParameterLayout,
		    bindingName,
		    bindingRecord.Kind);

		for (const RhiReflectedBindingLocation& reflectedLocation : reflectedLocations)
		{
			const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(reflectedLocation.VisibilityMask);
			bindingNames.emplace_back(bindingName);
			const std::uint32_t shaderRegister = reflectedLocation.BindingPoint.Binding;
			const std::uint32_t registerSpace = reflectedLocation.BindingPoint.Set;

			if (desc.InlineUniformDataAsPushConstants)
			{
				assert((bindingRecord.ValueSizeInBytes % sizeof(std::uint32_t)) == 0);
				const std::uint32_t bindingIndex = builder.AddRootConstants(
				    bindingRecord.ValueSizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t)),
				    shaderRegister,
				    registerSpace,
				    visibility);
				bindings.push_back(
				    CompiledBinding{
				        .Name = bindingNames.back().c_str(),
				        .Type = CompiledBindingType::PushConstants,
					    .SemanticKind = bindingRecord.Kind,
				        .BindingIndex = bindingIndex,
				        .BindingPoint = RhiBindingPoint{.Set = registerSpace, .Binding = shaderRegister},
				        .VisibilityMask = reflectedLocation.VisibilityMask,
				        .PushConstantCount = bindingRecord.ValueSizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t))});
				continue;
			}

			const std::uint32_t bindingIndex = builder.AddConstantBufferView(shaderRegister, registerSpace, visibility);
			bindings.push_back(
			    CompiledBinding{
			        .Name = bindingNames.back().c_str(),
			        .Type = CompiledBindingType::ConstantBuffer,
			        .SemanticKind = bindingRecord.Kind,
			        .BindingIndex = bindingIndex,
			        .BindingPoint = RhiBindingPoint{.Set = registerSpace, .Binding = shaderRegister},
			        .VisibilityMask = reflectedLocation.VisibilityMask,
			        .DescriptorCount = 1});
		}
	}

	static void CompileDescriptorTableBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<CompiledBinding>& bindings,
	    std::vector<std::string>& bindingNames,
	    const PassParameterDesc& bindingRecord,
	    std::string_view bindingName,
	    std::span<const ResolvedShader> shaders,
	    const PassParameterLayout& parameterLayout,
	    D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
	    CompiledBindingType bindingType)
	{
		const std::uint32_t descriptorCount = bindingRecord.ArrayCount;
		const std::vector<RhiReflectedBindingLocation> reflectedLocations = RhiShaderBindingReflection::ResolveLocations(
		    shaders,
		    parameterLayout,
		    bindingName,
		    bindingRecord.Kind);

		for (const RhiReflectedBindingLocation& reflectedLocation : reflectedLocations)
		{
			const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(reflectedLocation.VisibilityMask);
			bindingNames.emplace_back(bindingName);
			const std::uint32_t shaderRegister = reflectedLocation.BindingPoint.Binding;
			const std::uint32_t registerSpace = reflectedLocation.BindingPoint.Set;
			const std::uint32_t bindingIndex =
			    builder.AddDescriptorTable(rangeType, descriptorCount, shaderRegister, registerSpace, visibility);
			bindings.push_back(
			    CompiledBinding{
			        .Name = bindingNames.back().c_str(),
			        .Type = bindingType,
			        .SemanticKind = bindingRecord.Kind,
			        .BindingIndex = bindingIndex,
			        .BindingPoint = RhiBindingPoint{.Set = registerSpace, .Binding = shaderRegister},
			        .VisibilityMask = reflectedLocation.VisibilityMask,
			        .DescriptorCount = descriptorCount});
		}
	}

	static void CompileRootShaderResourceBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<CompiledBinding>& bindings,
	    std::vector<std::string>& bindingNames,
	    const PassParameterDesc& bindingRecord,
	    std::string_view bindingName,
	    std::span<const ResolvedShader> shaders,
	    const PassParameterLayout& parameterLayout)
	{
		const std::vector<RhiReflectedBindingLocation> reflectedLocations = RhiShaderBindingReflection::ResolveLocations(
		    shaders,
		    parameterLayout,
		    bindingName,
		    bindingRecord.Kind);

		for (const RhiReflectedBindingLocation& reflectedLocation : reflectedLocations)
		{
			const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(reflectedLocation.VisibilityMask);
			bindingNames.emplace_back(bindingName);
			const std::uint32_t shaderRegister = reflectedLocation.BindingPoint.Binding;
			const std::uint32_t registerSpace = reflectedLocation.BindingPoint.Set;
			const std::uint32_t bindingIndex = builder.AddShaderResourceView(shaderRegister, registerSpace, visibility);
			bindings.push_back(
			    CompiledBinding{
			        .Name = bindingNames.back().c_str(),
			        .Type = CompiledBindingType::AccelerationStructure,
			        .SemanticKind = bindingRecord.Kind,
			        .BindingIndex = bindingIndex,
			        .BindingPoint = RhiBindingPoint{.Set = registerSpace, .Binding = shaderRegister},
			        .VisibilityMask = reflectedLocation.VisibilityMask,
			        .DescriptorCount = 1});
		}
	}

	static D3D12_SHADER_VISIBILITY ToD3D12Visibility(ShaderStageMask visibilityMask) noexcept
	{
		const bool hasVertex = HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Vertex);
		const bool hasPixel = HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Pixel);
		const bool hasOther = HasAnyShaderStageMask(
		    visibilityMask,
		    ShaderStageMask::Geometry | ShaderStageMask::Hull | ShaderStageMask::Domain | ShaderStageMask::Compute);

		if (hasVertex && !hasPixel && !hasOther)
		{
			return D3D12_SHADER_VISIBILITY_VERTEX;
		}

		if (hasPixel && !hasVertex && !hasOther)
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
    RenderBindingLayout(parameterLayout, std::move(bindings), std::move(bindingNames)),
    m_rootSignature(std::move(rootSignature))
{
	assert(m_rootSignature != nullptr);
}

D3D12BindingLayout::~D3D12BindingLayout() noexcept = default;

D3D12RootSignature& D3D12BindingLayout::GetRootSignature() const noexcept
{
	assert(m_rootSignature != nullptr);
	return *m_rootSignature;
}

std::unique_ptr<D3D12BindingLayout> D3D12BindingLayoutCompiler::Compile(D3D12Rhi& rhi, const RenderBindingLayoutCompileDesc& desc)
{
	return D3D12BindingLayoutCompilerImpl::Compile(rhi, desc);
}
