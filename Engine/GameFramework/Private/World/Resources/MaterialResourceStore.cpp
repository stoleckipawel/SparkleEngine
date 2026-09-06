#include "PCH.h"

#include "World/Resources/MaterialResourceStore.h"

MaterialDesc MaterialResourceStore::CreateDefault()
{
	MaterialDesc material;
	material.name = "Scene_DefaultMaterial";
	return material;
}

MaterialHandle MaterialResourceStore::Append(std::vector<MaterialDesc> descriptions)
{
	if (descriptions.empty())
		return MaterialHandle::Invalid();
	const MaterialHandle base(static_cast<std::uint32_t>(m_descriptions.size()), m_generation);
	m_descriptions.reserve(m_descriptions.size() + descriptions.size());
	for (MaterialDesc& description : descriptions)
		m_descriptions.push_back(std::move(description));
	++m_contentRevision;
	return base;
}

MaterialHandle MaterialResourceStore::GetOrCreateDefault()
{
	if (!m_default.IsValid())
	{
		m_default = MaterialHandle(static_cast<std::uint32_t>(m_descriptions.size()), m_generation);
		m_descriptions.push_back(CreateDefault());
		++m_contentRevision;
	}
	return m_default;
}

RenderMaterialTable MaterialResourceStore::CaptureRenderTable() const
{
	return {.Values = m_descriptions, .Generation = m_generation};
}

bool MaterialResourceStore::Contains(MaterialHandle handle) const noexcept
{
	return handle.IsValid() && handle.GetGeneration() == m_generation && handle.GetIndex() < m_descriptions.size();
}
