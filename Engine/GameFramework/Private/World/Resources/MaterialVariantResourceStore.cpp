#include "PCH.h"

#include "World/Resources/MaterialVariantResourceStore.h"

#include "World/GameWorldState.h"

void MaterialVariantResourceStore::Append(std::vector<MaterialVariantDesc> variants, std::vector<MaterialVariantBinding> bindings)
{
	const MaterialVariantIndex base = static_cast<MaterialVariantIndex>(m_variants.size());
	for (MaterialVariantDesc& variant : variants)
		m_variants.push_back(std::move(variant));
	for (MaterialVariantBinding& binding : bindings)
	{
		if (binding.Variant == kInvalidMaterialVariantIndex || !binding.Entity.IsValid())
			continue;
		binding.Variant += base;
		m_bindings.push_back(binding);
	}
}

bool MaterialVariantResourceStore::Apply(MaterialVariantIndex index, ECS::GameWorldState& world)
{
	if (index >= m_variants.size())
		return false;
	bool applied = false;
	for (const MaterialVariantBinding& binding : m_bindings)
		if (binding.Variant == index)
			applied = world.WriteMeshMaterial(binding.Entity, binding.Material) || applied;
	if (applied)
		m_active = index;
	return applied;
}

std::string_view MaterialVariantResourceStore::GetName(std::size_t index) const noexcept
{
	return index < m_variants.size() ? std::string_view(m_variants[index].name) : std::string_view{};
}
