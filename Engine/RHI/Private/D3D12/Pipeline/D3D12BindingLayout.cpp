#include "PCH.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"

#include "D3D12/Pipeline/D3D12RootSignatureBuilder.h"

#include "Shaders/CookedShaderPackageCache.h"
#include "ShaderParameters/PassParameterLayout.h"

#include <algorithm>
#include <vector>
#include <cassert>
#include <optional>
#include <string_view>

class D3D12BindingLayoutCompilerImpl final
{
  public:
	static std::unique_ptr<D3D12BindingLayout> Compile(D3D12Rhi& rhi, const RenderBindingLayoutCompileDesc& desc)
	{
		assert(desc.ParameterLayout != nullptr);
		assert(desc.ShaderPackage != nullptr);

		const LoadedShaderPackage& shaderPackage = *desc.ShaderPackage;
		const std::vector<CookedShaderBindingRecord>& bindingRecords = shaderPackage.GetBindingRecords();

		D3D12RootSignatureBuilder builder;
		builder.SetFlags(
		    desc.AllowInputAssemblerInputLayout ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		                                        : D3D12_ROOT_SIGNATURE_FLAG_NONE);

		std::vector<CompiledBinding> bindings;
		std::vector<std::string> bindingNames;
		bindings.reserve(bindingRecords.size() * static_cast<std::size_t>(ShaderStage::Count));
		bindingNames.reserve(bindingRecords.size() * static_cast<std::size_t>(ShaderStage::Count));

		std::uint32_t cbvRegister = 0;
		std::uint32_t srvRegister = 0;
		std::uint32_t uavRegister = 0;
		std::uint32_t samplerRegister = 0;

		for (const CookedShaderBindingRecord& bindingRecord : bindingRecords)
		{
			const std::string_view bindingName = shaderPackage.ResolveString(bindingRecord.Name);
			assert(!bindingName.empty());
			const PassParameterDesc* parameterDesc = FindParameter(*desc.ParameterLayout, bindingName);

			switch (bindingRecord.SemanticKind)
			{
				case ShaderParameterSemanticKind::UniformData:
					CompileUniformBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    shaderPackage,
					    parameterDesc,
					    desc,
					    cbvRegister);
					break;
				case ShaderParameterSemanticKind::ReadTexture:
				case ShaderParameterSemanticKind::ReadBuffer:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    shaderPackage,
					    parameterDesc,
					    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					    CompiledBindingType::ReadOnlyResourceTable,
					    srvRegister);
					break;
				case ShaderParameterSemanticKind::AccelerationStructure:
					CompileRootShaderResourceBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    shaderPackage,
					    parameterDesc,
					    srvRegister);
					break;
				case ShaderParameterSemanticKind::RWTexture:
				case ShaderParameterSemanticKind::RWBuffer:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    shaderPackage,
					    parameterDesc,
					    D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
					    CompiledBindingType::ReadWriteResourceTable,
					    uavRegister);
					break;
				case ShaderParameterSemanticKind::SamplerSet:
					CompileDescriptorTableBinding(
					    builder,
					    bindings,
					    bindingNames,
					    bindingRecord,
					    bindingName,
					    shaderPackage,
					    parameterDesc,
					    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
					    CompiledBindingType::SamplerTable,
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
	struct ReflectedBindingLocation final
	{
		std::uint32_t Register = 0;
		std::uint32_t RegisterSpace = 0;
		ShaderStageMask VisibilityMask = ShaderStageMask::None;
	};

	static const PassParameterDesc* FindParameter(const PassParameterLayout& layout, std::string_view name) noexcept
	{
		for (const PassParameterDesc& parameter : layout.GetParameters())
		{
			if (parameter.Name == name)
			{
				return &parameter;
			}
		}

		return nullptr;
	}

	static bool ResourceKindMatches(CookedShaderResourceKind kind, ShaderParameterSemanticKind semanticKind) noexcept
	{
		switch (semanticKind)
		{
			case ShaderParameterSemanticKind::UniformData:
				return kind == CookedShaderResourceKind::ConstantBuffer || kind == CookedShaderResourceKind::PushConstantBlock;
			case ShaderParameterSemanticKind::ReadTexture:
				return kind == CookedShaderResourceKind::Texture;
			case ShaderParameterSemanticKind::ReadBuffer:
				return kind == CookedShaderResourceKind::StructuredBuffer || kind == CookedShaderResourceKind::ByteAddressBuffer ||
				       kind == CookedShaderResourceKind::TypedBuffer;
			case ShaderParameterSemanticKind::AccelerationStructure:
				return kind == CookedShaderResourceKind::AccelerationStructure;
			case ShaderParameterSemanticKind::RWTexture:
				return kind == CookedShaderResourceKind::RWTexture;
			case ShaderParameterSemanticKind::RWBuffer:
				return kind == CookedShaderResourceKind::RWStructuredBuffer || kind == CookedShaderResourceKind::RWByteAddressBuffer ||
				       kind == CookedShaderResourceKind::RWTypedBuffer;
			case ShaderParameterSemanticKind::SamplerSet:
				return kind == CookedShaderResourceKind::Sampler;
			default:
				return false;
		}
	}

	static bool StageMaskContains(ShaderStageMask mask, ShaderStage stage) noexcept
	{
		return HasAnyShaderStageMask(mask, ToShaderStageMask(stage));
	}

	static std::vector<ReflectedBindingLocation> FindReflectedBindingLocations(
	    const LoadedShaderPackage& shaderPackage,
	    const CookedShaderBindingRecord& bindingRecord,
	    const PassParameterDesc* parameterDesc)
	{
		if (parameterDesc == nullptr)
		{
			return {};
		}

		const std::string_view shaderName = parameterDesc->GetShaderName();
		if (shaderName.empty())
		{
			return {};
		}

		const std::vector<CookedShaderBinaryRecord>& binaryRecords = shaderPackage.GetBinaryRecords();
		const std::vector<CookedShaderReflectionRecord>& reflectionRecords = shaderPackage.GetReflectionRecords();
		const std::vector<CookedShaderResourceBindingRecord>& resourceBindings = shaderPackage.GetResourceBindings();
		std::vector<ReflectedBindingLocation> locations;
		for (std::size_t reflectionIndex = 0; reflectionIndex < reflectionRecords.size() && reflectionIndex < binaryRecords.size();
		     ++reflectionIndex)
		{
			const CookedShaderBinaryRecord& binaryRecord = binaryRecords[reflectionIndex];
			if (binaryRecord.Format != CookedShaderBinaryFormat::Dxil)
			{
				continue;
			}

			const ShaderStage stage = binaryRecord.Stage;
			if (!StageMaskContains(bindingRecord.VisibilityMask, stage))
			{
				continue;
			}

			const CookedShaderReflectionRecord& reflection = reflectionRecords[reflectionIndex];
			for (std::uint32_t resourceIndex = 0; resourceIndex < reflection.ResourceBindingCount; ++resourceIndex)
			{
				const std::uint32_t bindingIndex = reflection.ResourceBindingOffset + resourceIndex;
				if (bindingIndex >= resourceBindings.size())
				{
					break;
				}

				const CookedShaderResourceBindingRecord& resourceBinding = resourceBindings[bindingIndex];
				if (!ResourceKindMatches(resourceBinding.Kind, bindingRecord.SemanticKind))
				{
					continue;
				}

				const std::string_view resourceName =
				    shaderPackage.ResolveString(CookedShaderStringRef{resourceBinding.NameOffsetInBytes, resourceBinding.NameSizeInBytes});
				if (resourceName != shaderName)
				{
					continue;
				}

				const auto existing = std::ranges::find_if(
				    locations,
				    [&resourceBinding](const ReflectedBindingLocation& location)
				    {
					    return location.Register == resourceBinding.Slot && location.RegisterSpace == resourceBinding.Set;
				    });
				if (existing != locations.end())
				{
					existing->VisibilityMask |= ToShaderStageMask(stage);
				}
				else
				{
					locations.push_back(ReflectedBindingLocation{resourceBinding.Slot, resourceBinding.Set, ToShaderStageMask(stage)});
				}
			}
		}

		return locations;
	}

	static void CompileUniformBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<CompiledBinding>& bindings,
	    std::vector<std::string>& bindingNames,
	    const CookedShaderBindingRecord& bindingRecord,
	    std::string_view bindingName,
	    const LoadedShaderPackage& shaderPackage,
	    const PassParameterDesc* parameterDesc,
	    const RenderBindingLayoutCompileDesc& desc,
	    std::uint32_t& cbvRegister)
	{
		assert(bindingRecord.ValueSizeInBytes > 0);
		std::vector<ReflectedBindingLocation> reflectedLocations =
		    FindReflectedBindingLocations(shaderPackage, bindingRecord, parameterDesc);
		assert(!reflectedLocations.empty() && "Reflected shader binding is required for uniform parameters.");
		if (reflectedLocations.empty())
		{
			reflectedLocations.push_back(ReflectedBindingLocation{cbvRegister, 0u, bindingRecord.VisibilityMask});
		}

		for (const ReflectedBindingLocation& reflectedLocation : reflectedLocations)
		{
			const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(reflectedLocation.VisibilityMask);
			bindingNames.emplace_back(bindingName);
			const std::uint32_t shaderRegister = reflectedLocation.Register;
			const std::uint32_t registerSpace = reflectedLocation.RegisterSpace;

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
				        .BindingIndex = bindingIndex,
				        .BindingPoint = RhiBindingPoint{.Set = registerSpace, .Binding = shaderRegister},
				        .VisibilityMask = reflectedLocation.VisibilityMask,
				        .PushConstantCount = bindingRecord.ValueSizeInBytes / static_cast<std::uint32_t>(sizeof(std::uint32_t))});
				cbvRegister = std::max(cbvRegister, shaderRegister + 1u);
				continue;
			}

			const std::uint32_t bindingIndex = builder.AddConstantBufferView(shaderRegister, registerSpace, visibility);
			bindings.push_back(
			    CompiledBinding{
			        .Name = bindingNames.back().c_str(),
			        .Type = CompiledBindingType::ConstantBuffer,
			        .BindingIndex = bindingIndex,
			        .BindingPoint = RhiBindingPoint{.Set = registerSpace, .Binding = shaderRegister},
			        .VisibilityMask = reflectedLocation.VisibilityMask,
			        .DescriptorCount = 1});
			cbvRegister = std::max(cbvRegister, shaderRegister + 1u);
		}
	}

	static void CompileDescriptorTableBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<CompiledBinding>& bindings,
	    std::vector<std::string>& bindingNames,
	    const CookedShaderBindingRecord& bindingRecord,
	    std::string_view bindingName,
	    const LoadedShaderPackage& shaderPackage,
	    const PassParameterDesc* parameterDesc,
	    D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
	    CompiledBindingType bindingType,
	    std::uint32_t& nextShaderRegister)
	{
		const std::uint32_t descriptorCount = bindingRecord.ArrayCount;
		std::vector<ReflectedBindingLocation> reflectedLocations =
		    FindReflectedBindingLocations(shaderPackage, bindingRecord, parameterDesc);
		assert(!reflectedLocations.empty() && "Reflected shader binding is required for descriptor table parameters.");
		if (reflectedLocations.empty())
		{
			reflectedLocations.push_back(ReflectedBindingLocation{nextShaderRegister, 0u, bindingRecord.VisibilityMask});
		}

		for (const ReflectedBindingLocation& reflectedLocation : reflectedLocations)
		{
			const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(reflectedLocation.VisibilityMask);
			bindingNames.emplace_back(bindingName);
			const std::uint32_t shaderRegister = reflectedLocation.Register;
			const std::uint32_t registerSpace = reflectedLocation.RegisterSpace;
			const std::uint32_t bindingIndex =
			    builder.AddDescriptorTable(rangeType, descriptorCount, shaderRegister, registerSpace, visibility);
			bindings.push_back(
			    CompiledBinding{
			        .Name = bindingNames.back().c_str(),
			        .Type = bindingType,
			        .BindingIndex = bindingIndex,
			        .BindingPoint = RhiBindingPoint{.Set = registerSpace, .Binding = shaderRegister},
			        .VisibilityMask = reflectedLocation.VisibilityMask,
			        .DescriptorCount = descriptorCount});
			nextShaderRegister = std::max(nextShaderRegister, shaderRegister + descriptorCount);
		}
	}

	static void CompileRootShaderResourceBinding(
	    D3D12RootSignatureBuilder& builder,
	    std::vector<CompiledBinding>& bindings,
	    std::vector<std::string>& bindingNames,
	    const CookedShaderBindingRecord& bindingRecord,
	    std::string_view bindingName,
	    const LoadedShaderPackage& shaderPackage,
	    const PassParameterDesc* parameterDesc,
	    std::uint32_t& nextShaderRegister)
	{
		std::vector<ReflectedBindingLocation> reflectedLocations =
		    FindReflectedBindingLocations(shaderPackage, bindingRecord, parameterDesc);
		assert(!reflectedLocations.empty() && "Reflected shader binding is required for root SRV parameters.");
		if (reflectedLocations.empty())
		{
			reflectedLocations.push_back(ReflectedBindingLocation{nextShaderRegister, 0u, bindingRecord.VisibilityMask});
		}

		for (const ReflectedBindingLocation& reflectedLocation : reflectedLocations)
		{
			const D3D12_SHADER_VISIBILITY visibility = ToD3D12Visibility(reflectedLocation.VisibilityMask);
			bindingNames.emplace_back(bindingName);
			const std::uint32_t shaderRegister = reflectedLocation.Register;
			const std::uint32_t registerSpace = reflectedLocation.RegisterSpace;
			const std::uint32_t bindingIndex = builder.AddShaderResourceView(shaderRegister, registerSpace, visibility);
			bindings.push_back(
			    CompiledBinding{
			        .Name = bindingNames.back().c_str(),
			        .Type = CompiledBindingType::ReadOnlyAddress,
			        .BindingIndex = bindingIndex,
			        .BindingPoint = RhiBindingPoint{.Set = registerSpace, .Binding = shaderRegister},
			        .VisibilityMask = reflectedLocation.VisibilityMask,
			        .DescriptorCount = 1});
			nextShaderRegister = std::max(nextShaderRegister, shaderRegister + 1u);
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