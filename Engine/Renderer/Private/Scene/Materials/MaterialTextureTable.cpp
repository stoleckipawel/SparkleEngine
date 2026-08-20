#include "PCH.h"
#include "Scene/Materials/MaterialTextureTable.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <limits>
#include <utility>

static const auto g_materialTextureTableLogger = Logging::GetOrCreateLogger("Renderer.MaterialTextureTable");

void MaterialTextureTable::Reset() noexcept
{
	m_bindingSet.reset();
	m_textureViews.clear();
}

std::uint32_t MaterialTextureTable::GetOrAddTextureIndex(RhiResourceViewHandle textureView)
{
	if (!textureView)
	{
		Diagnostics::Fatal(g_materialTextureTableLogger, __FILE__, __LINE__, "Material texture table received an invalid texture view.");
	}

	for (std::uint32_t index = 0u; index < static_cast<std::uint32_t>(m_textureViews.size()); ++index)
	{
		if (m_textureViews[index] == textureView)
		{
			return index;
		}
	}

	if (m_textureViews.size() >= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
	{
		Diagnostics::Fatal(
		    g_materialTextureTableLogger,
		    __FILE__,
		    __LINE__,
		    "Material texture table exceeded the 32-bit shader index range.");
	}

	const std::uint32_t index = static_cast<std::uint32_t>(m_textureViews.size());
	m_textureViews.push_back(textureView);
	return index;
}

void MaterialTextureTable::BuildBindingSet(RenderHardwareInterface& renderHardwareInterface)
{
	m_bindingSet.reset();
	if (m_textureViews.empty())
	{
		Diagnostics::Fatal(g_materialTextureTableLogger, __FILE__, __LINE__, "Material texture table has no texture views.");
	}
	if (m_textureViews.size() > MaterialTextureTableFixedCapacity)
	{
		Diagnostics::Fatal(
		    g_materialTextureTableLogger,
		    __FILE__,
		    __LINE__,
		    "Material texture table exceeded the shader descriptor capacity.");
	}

	for (const RhiResourceViewHandle textureView : m_textureViews)
	{
		if (!textureView)
		{
			Diagnostics::Fatal(
			    g_materialTextureTableLogger,
			    __FILE__,
			    __LINE__,
			    "Material texture table contains an invalid texture view.");
		}
	}

	auto bindingSet = renderHardwareInterface.GetDescriptorService().CreateBindingSet(
	    RenderBindingSetDesc{
	        .DescriptorType = ERhiDescriptorAllocatorType::ShaderResource,
	        .DescriptorCount = static_cast<std::uint32_t>(m_textureViews.size())});
	if (!bindingSet || !*bindingSet)
	{
		Diagnostics::Fatal(g_materialTextureTableLogger, __FILE__, __LINE__, "Material texture descriptor-table allocation failed.");
	}

	for (std::uint32_t index = 0u; index < static_cast<std::uint32_t>(m_textureViews.size()); ++index)
	{
		if (!bindingSet->WriteResourceView(index, m_textureViews[index]))
		{
			Diagnostics::Fatal(g_materialTextureTableLogger, __FILE__, __LINE__, "Material texture descriptor write failed.");
		}
	}

	m_bindingSet = std::move(bindingSet);
}

RhiDescriptorTableBinding MaterialTextureTable::GetTableBinding() const noexcept
{
	return m_bindingSet != nullptr ? m_bindingSet->GetTableBinding(0u) : RhiDescriptorTableBinding{};
}
