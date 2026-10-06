#pragma once

#include "../../../RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "ShaderParameterFields.h"

#include <cassert>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>
#include <span>
#include <cstring>

template <typename TParameters> class ShaderParameterStructBuilder;

template <typename TParameters> class ShaderParameterStructRegistry final
{
public:
	using DescribeField = std::function<void(ShaderParameterStructBuilder<TParameters>&)>;

	static void AddField(std::string name, DescribeField describe)
	{
		auto& fields = MutableFields();
		const auto existing = std::ranges::find_if(fields, [&name](const RegisteredField& field) { return field.Name == name; });
		if (existing == fields.end())
		{
			fields.push_back(RegisteredField{std::move(name), std::move(describe)});
		}
	}

	static void Describe(ShaderParameterStructBuilder<TParameters>& builder)
	{
		for (const RegisteredField& field : MutableFields())
		{
			field.Describe(builder);
		}
	}

private:
	struct RegisteredField final
	{
		std::string Name;
		DescribeField Describe;
	};

	static std::vector<RegisteredField>& MutableFields()
	{
		static std::vector<RegisteredField> fields;
		return fields;
	}
};

template <typename TParameters, typename TField> class ShaderParameterFieldAutoRegister final
{
public:
	ShaderParameterFieldAutoRegister(
	    const char* name,
	    TField TParameters::* member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::All,
	    bool usesGraphResource = ShaderParameterFieldTraits<TField>::UsesGraphResource)
	{
		ShaderParameterStructRegistry<TParameters>::AddField(
		    name != nullptr ? name : "",
		    [name, member, visibility, usesGraphResource](ShaderParameterStructBuilder<TParameters>& builder)
		    { builder.Add(name, member, visibility, usesGraphResource); });
	}
};

template <typename TParameters, typename TValue> class ShaderParameterValueAutoRegister final
{
public:
	ShaderParameterValueAutoRegister(const char* name, TValue TParameters::* member)
	{
		ShaderParameterStructRegistry<TParameters>::AddField(
		    name,
		    [name, member](ShaderParameterStructBuilder<TParameters>& builder) { builder.Value(name, member); });
	}
};

template <typename TParameters> struct ShaderParameterStructBinding
{
	std::string Name;
	std::function<bool(PassParameterSet&, const char*, const TParameters&, std::vector<std::byte>&)> Bind;
};

template <typename TParameters> class ShaderParameterStructMetadata final
{
public:
	ShaderParameterStructMetadata() = default;

	ShaderParameterStructMetadata(
	    PassParameterLayout layout,
	    std::vector<ShaderParameterStructBinding<TParameters>> bindings,
	    std::vector<bool> graphResourceParameters) :
	    m_layout(std::move(layout)),
	    m_bindings(std::move(bindings)),
	    m_graphResourceParameters(std::move(graphResourceParameters))
	{
		assert(m_graphResourceParameters.size() == m_bindings.size());
	}

	const PassParameterLayout& GetLayout() const noexcept { return m_layout; }

	const std::vector<ShaderParameterStructBinding<TParameters>>& GetBindings() const noexcept { return m_bindings; }
	const std::vector<bool>& GetGraphResourceParameters() const noexcept { return m_graphResourceParameters; }

	bool Commit(
	    const TParameters& parameters,
	    PassParameterSet& parameterSet,
	    std::vector<std::vector<std::byte>>& storage,
	    std::vector<std::string>* failedBindings = nullptr) const
	{
		storage.resize(m_bindings.size());
		parameterSet.ClearBindings();
		if (failedBindings != nullptr)
		{
			failedBindings->clear();
		}

		bool succeeded = true;
		for (std::size_t index = 0; index < m_bindings.size(); ++index)
		{
			const auto& binding = m_bindings[index];
			if (!binding.Bind || !binding.Bind(parameterSet, binding.Name.c_str(), parameters, storage[index]))
			{
				succeeded = false;
				if (failedBindings != nullptr)
				{
					failedBindings->push_back(binding.Name);
				}
			}
		}

		if (!parameterSet.HasAllRequiredBindings())
		{
			succeeded = false;
			if (failedBindings != nullptr)
			{
				for (const std::string& missingBinding : parameterSet.GetMissingBindings())
				{
					if (std::find(failedBindings->begin(), failedBindings->end(), missingBinding) == failedBindings->end())
					{
						failedBindings->push_back(missingBinding);
					}
				}
			}
		}

		return succeeded;
	}

private:
	PassParameterLayout m_layout;
	std::vector<ShaderParameterStructBinding<TParameters>> m_bindings;
	std::vector<bool> m_graphResourceParameters;
};

template <typename TParameters> class ShaderParameterStructBuilder final
{
public:
	explicit ShaderParameterStructBuilder(const char* debugName) :
	    m_layout(debugName)
	{
	}

	template <typename TField> std::uint32_t Add(
	    const char* name,
	    TField TParameters::* member,
	    ShaderStageVisibility visibility,
	    bool usesGraphResource = ShaderParameterFieldTraits<TField>::UsesGraphResource)
	{
		return AddField<typename ShaderParameterFieldTraits<TField>::Semantic>(name, member, visibility, usesGraphResource);
	}

	template <typename TField> std::uint32_t RenderTarget(
	    const char* name,
	    TField TParameters::* member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::AllGraphics)
	{
		return AddField<::RenderTarget>(name, member, visibility);
	}

	template <typename TField> std::uint32_t DepthTarget(
	    const char* name,
	    TField TParameters::* member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::AllGraphics)
	{
		return AddField<::DepthTarget>(name, member, visibility);
	}

	template <typename TNestedParameters>
	void Include(TNestedParameters TParameters::* member, ShaderStageVisibility visibility = ShaderStageVisibility::None)
	{
		static const ShaderParameterStructMetadata<TNestedParameters> nestedMetadata =
		    ShaderParameterStructBuilder<TNestedParameters>::BuildMetadata("NestedShaderParameters");
		const auto& nestedLayout = nestedMetadata.GetLayout().GetParameters();
		const auto& nestedBindings = nestedMetadata.GetBindings();
		const auto& nestedGraphResources = nestedMetadata.GetGraphResourceParameters();
		assert(nestedLayout.size() == nestedBindings.size());
		assert(nestedBindings.size() == nestedGraphResources.size());

		for (std::size_t index = 0; index < nestedBindings.size(); ++index)
		{
			PassParameterDesc parameter = nestedLayout[index];
			if (visibility != ShaderStageVisibility::None)
			{
				parameter.Visibility = visibility;
			}
			const bool alreadyIncluded = m_layout.HasParameter(parameter.Name);
			m_layout.AddParameter(std::move(parameter));
			if (alreadyIncluded)
			{
				continue;
			}

			const auto* nestedBinding = &nestedBindings[index];
			m_bindings.push_back(
			    ShaderParameterStructBinding<TParameters>{
			        .Name = nestedBinding->Name,
			        .Bind = [member, nestedBinding](
			                    PassParameterSet& parameterSet,
			                    const char* bindingName,
			                    const TParameters& parameters,
			                    std::vector<std::byte>& storage)
			        { return nestedBinding->Bind(parameterSet, bindingName, parameters.*member, storage); }});
			m_graphResourceParameters.push_back(nestedGraphResources[index]);
		}
	}

	const PassParameterLayout& GetLayout() const noexcept { return m_layout; }

	template <typename TValue> void Value(const char* name, TValue TParameters::* member)
	{
		const auto descriptor = TParameters::GetShaderParameterStructDescriptor();
		const auto value = std::ranges::find_if(descriptor.Values, [name](const auto& field) { return field.Name == name; });
		if (value == descriptor.Values.end() || value->SizeInBytes != sizeof(TValue))
		{
			throw std::logic_error("Shader value metadata does not match its parameter field.");
		}
		m_valueWriters.push_back([member, offset = value->OffsetInBytes](const TParameters& parameters, std::span<std::byte> storage)
		    { std::memcpy(storage.data() + offset, &(parameters.*member), sizeof(TValue)); });
	}

	ShaderParameterStructMetadata<TParameters> Build() &&
	{
		if constexpr (requires { TParameters::GetShaderParameterStructDescriptor(); })
		{
			if (!m_valueWriters.empty())
			{
				const auto descriptor = TParameters::GetShaderParameterStructDescriptor();
				const auto& block = descriptor.Fields.back();
				m_layout.AddParameter(
				    PassParameterDesc{
				        .Name = block.Name,
				        .Kind = block.SemanticKind,
				        .ResourceDomain = block.ResourceDomain,
				        .Visibility = m_valueVisibility,
				        .ValueSizeInBytes = block.ValueSizeInBytes,
				        .ValueLayoutHash = block.ValueLayoutHash});
				m_graphResourceParameters.push_back(false);
				m_bindings.push_back(
				    {block.Name,
				        [writers = std::move(m_valueWriters), size = block.ValueSizeInBytes](
				            PassParameterSet& parameterSet,
				            const char* name,
				            const TParameters& parameters,
				            std::vector<std::byte>& storage)
				        {
					        storage.assign(size, std::byte{});
					        for (const auto& write : writers)
					        {
						        write(parameters, storage);
					        }
					        return parameterSet.SetUniformDataBytes(name, storage.data(), size);
				        }});
			}
		}
		return ShaderParameterStructMetadata<TParameters>(std::move(m_layout), std::move(m_bindings), std::move(m_graphResourceParameters));
	}

	static ShaderParameterStructMetadata<TParameters> BuildMetadata(
	    const char* debugName,
	    ShaderStageVisibility visibility = ShaderStageVisibility::None)
	{
		ShaderParameterStructBuilder builder(debugName);
		if constexpr (requires { TParameters::Describe(builder); })
		{
			TParameters::Describe(builder);
		}
		else
		{
			ShaderParameterStructRegistry<TParameters>::Describe(builder);
		}
		if (visibility != ShaderStageVisibility::None)
		{
			builder.m_layout.SetAllVisibility(visibility);
			builder.m_valueVisibility = visibility;
		}
		return std::move(builder).Build();
	}

private:
	template <typename TExpectedSemantic, typename TField> std::uint32_t AddField(
	    const char* name,
	    TField TParameters::* member,
	    ShaderStageVisibility visibility,
	    bool usesGraphResource = ShaderParameterFieldTraits<TField>::UsesGraphResource)
	{
		static_assert(IsShaderParameterFieldV<TField>, "Parameter registration requires a typed shader parameter field.");

		using ActualSemantic = typename ShaderParameterFieldTraits<TField>::Semantic;
		static_assert(
		    std::is_same_v<ActualSemantic, TExpectedSemantic>,
		    "Builder registration method does not match the shader parameter field type.");

		m_bindings.push_back(
		    ShaderParameterStructBinding<TParameters>{
		        .Name = name != nullptr ? name : "",
		        .Bind = [member](
		                    PassParameterSet& parameterSet,
		                    const char* bindingName,
		                    const TParameters& parameters,
		                    std::vector<std::byte>&) { return BindParameterField(parameterSet, bindingName, parameters.*member); }});
		m_graphResourceParameters.push_back(usesGraphResource);

		return m_layout.Add<ActualSemantic>(name, visibility, ShaderParameterFieldTraits<TField>::FieldArrayCount);
	}

	ShaderStageVisibility m_valueVisibility = ShaderStageVisibility::All;
	PassParameterLayout m_layout;
	std::vector<ShaderParameterStructBinding<TParameters>> m_bindings;
	std::vector<bool> m_graphResourceParameters;
	std::vector<std::function<void(const TParameters&, std::span<std::byte>)>> m_valueWriters;
};
