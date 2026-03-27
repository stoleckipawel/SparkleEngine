#pragma once

#include "PassParameterLayout.h"
#include "ShaderParameterFields.h"

#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

template <typename TParameters>
struct ShaderParameterStructBinding
{
	std::string Name;
	std::function<bool(PassParameterSet&, const char*, const TParameters&)> Bind;
};

template <typename TParameters>
class ShaderParameterStructMetadata final
{
  public:
	ShaderParameterStructMetadata() = default;

	ShaderParameterStructMetadata(
	    PassParameterLayout layout,
	    std::vector<ShaderParameterStructBinding<TParameters>> bindings) :
		m_layout(std::move(layout)),
		m_bindings(std::move(bindings))
	{
	}

	const PassParameterLayout& GetLayout() const noexcept
	{
		return m_layout;
	}

	const std::vector<ShaderParameterStructBinding<TParameters>>& GetBindings() const noexcept
	{
		return m_bindings;
	}

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
};

template <typename TParameters>
class ShaderParameterStructBuilder final
{
  public:
	explicit ShaderParameterStructBuilder(const char* debugName) : m_layout(debugName) {}

	template <typename TField>
	std::uint32_t Add(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::All)
	{
		return AddField<typename ShaderParameterFieldTraits<TField>::Semantic>(name, member, visibility);
	}

	template <typename TField>
	std::uint32_t ReadTexture(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::All)
	{
		return AddField<::ReadTexture>(name, member, visibility);
	}

	template <typename TField>
	std::uint32_t RWTexture(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::All)
	{
		return AddField<::RWTexture>(name, member, visibility);
	}

	template <typename TField>
	std::uint32_t RenderTarget(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::AllGraphics)
	{
		return AddField<::RenderTarget>(name, member, visibility);
	}

	template <typename TField>
	std::uint32_t DepthTarget(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::AllGraphics)
	{
		return AddField<::DepthTarget>(name, member, visibility);
	}

	template <typename TField>
	std::uint32_t ReadBuffer(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::All)
	{
		return AddField<::ReadBuffer>(name, member, visibility);
	}

	template <typename TField>
	std::uint32_t RWBuffer(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::All)
	{
		return AddField<::RWBuffer>(name, member, visibility);
	}

	template <typename TField>
	std::uint32_t Uniform(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::All)
	{
		static_assert(IsShaderParameterFieldV<TField>, "Uniform registration requires a typed shader parameter field.");
		using ActualSemantic = typename ShaderParameterFieldTraits<TField>::Semantic;
		static_assert(
		    std::is_same_v<ShaderParameterSemanticTraits<ActualSemantic>, ShaderParameterSemanticTraits<ActualSemantic>>,
		    "Uniform registration requires a uniform field type.");
		static_assert(ShaderParameterSemanticTraits<ActualSemantic>::Kind == ShaderParameterSemanticKind::UniformData,
		    "Uniform registration requires a ShaderUniform<T> field.");
		return AddField<ActualSemantic>(name, member, visibility);
	}

	template <typename TField>
	std::uint32_t Sampler(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility = ShaderStageVisibility::All)
	{
		return AddField<::SamplerSet>(name, member, visibility);
	}

	const PassParameterLayout& GetLayout() const noexcept
	{
		return m_layout;
	}

	ShaderParameterStructMetadata<TParameters> Build() const
	{
		return ShaderParameterStructMetadata<TParameters>(m_layout, m_bindings);
	}

	static ShaderParameterStructMetadata<TParameters> BuildMetadata(const char* debugName)
	{
		ShaderParameterStructBuilder builder(debugName);
		TParameters::Describe(builder);
		return builder.Build();
	}

	static PassParameterLayout BuildLayout(const char* debugName)
	{
		return BuildMetadata(debugName).GetLayout();
	}

  private:
	template <typename TExpectedSemantic, typename TField>
	std::uint32_t AddField(
	    const char* name,
	    TField TParameters::*member,
	    ShaderStageVisibility visibility)
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
		        {
			        return BindParameterField(parameterSet, bindingName, parameters.*member);
		        }});

		return m_layout.Add<ActualSemantic>(name, visibility, ShaderParameterFieldTraits<TField>::FieldArrayCount);
	}

	PassParameterLayout m_layout;
	std::vector<ShaderParameterStructBinding<TParameters>> m_bindings;
};