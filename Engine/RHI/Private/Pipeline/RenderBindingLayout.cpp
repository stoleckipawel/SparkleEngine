#include "PCH.h"

#include "Pipeline/RhiPipelineDesc.h"

#include "ShaderParameters/PassParameterLayout.h"

#include <cassert>
#include <string_view>
#include <utility>

RenderBindingLayout::RenderBindingLayout(
    const PassParameterLayout& parameterLayout,
    std::vector<CompiledBinding> bindings,
    std::vector<std::string> bindingNames) noexcept :
    m_parameterLayout(&parameterLayout),
    m_bindings(std::move(bindings)),
    m_bindingNames(std::move(bindingNames))
{
	assert(m_bindings.size() == m_bindingNames.size());
	for (std::size_t bindingIndex = 0; bindingIndex < m_bindings.size(); ++bindingIndex)
	{
		m_bindings[bindingIndex].Name = m_bindingNames[bindingIndex].c_str();
	}
}

RenderBindingLayout::~RenderBindingLayout() noexcept = default;

const PassParameterLayout& RenderBindingLayout::GetParameterLayout() const noexcept
{
	assert(m_parameterLayout != nullptr);
	return *m_parameterLayout;
}

const CompiledBinding* RenderBindingLayout::GetBindings() const noexcept
{
	return m_bindings.data();
}

std::size_t RenderBindingLayout::GetBindingCount() const noexcept
{
	return m_bindings.size();
}

const CompiledBinding* RenderBindingLayout::FindBinding(const char* name) const noexcept
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
