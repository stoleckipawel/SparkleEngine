#pragma once

#include "../../../RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "ShaderParameterFields.h"

#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

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

template <typename TParameters> struct ShaderParameterStructBinding
{
	std::string Name;
	std::function<bool(PassParameterSet&, const char*, const TParameters&)> Bind;
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

	bool Commit(const TParameters& parameters, PassParameterSet& parameterSet, std::vector<std::string>* failedBindings = nullptr) const
	{
		parameterSet.ClearBindings();
		if (failedBindings != nullptr)
		{
			failedBindings->clear();
		}

		bool succeeded = true;
		for (const ShaderParameterStructBinding<TParameters>& binding : m_bindings)
		{
			if (!binding.Bind || !binding.Bind(parameterSet, binding.Name.c_str(), parameters))
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

			const ShaderParameterStructBinding<TNestedParameters> nestedBinding = nestedBindings[index];
			m_bindings.push_back(
			    ShaderParameterStructBinding<TParameters>{
			        .Name = nestedBinding.Name,
			        .Bind = [member, nestedBinding](PassParameterSet& parameterSet, const char* bindingName, const TParameters& parameters)
			        { return nestedBinding.Bind(parameterSet, bindingName, parameters.*member); }});
			m_graphResourceParameters.push_back(nestedGraphResources[index]);
		}
	}

	const PassParameterLayout& GetLayout() const noexcept { return m_layout; }

	ShaderParameterStructMetadata<TParameters> Build() const
	{
		return ShaderParameterStructMetadata<TParameters>(m_layout, m_bindings, m_graphResourceParameters);
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
		}
		return builder.Build();
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
		        .Bind = [member](PassParameterSet& parameterSet, const char* bindingName, const TParameters& parameters)
		        { return BindParameterField(parameterSet, bindingName, parameters.*member); }});
		m_graphResourceParameters.push_back(usesGraphResource);

		return m_layout.Add<ActualSemantic>(name, visibility, ShaderParameterFieldTraits<TField>::FieldArrayCount);
	}

	PassParameterLayout m_layout;
	std::vector<ShaderParameterStructBinding<TParameters>> m_bindings;
	std::vector<bool> m_graphResourceParameters;
};
