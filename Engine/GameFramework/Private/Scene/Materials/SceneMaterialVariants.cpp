#include "PCH.h"

#include "Scene/Materials/SceneMaterialVariants.h"

#include "Scene/Meshes/SceneMeshes.h"

#include <utility>

void SceneMaterialVariants::AppendVariants(
    std::vector<SceneMaterialVariantDesc>&& variants,
    std::vector<SceneMaterialVariantBinding>&& bindings)
{
	if (variants.empty())
	{
		return;
	}

	const auto variantBaseIndex = static_cast<SceneMaterialVariantIndex>(m_variants.size());
	m_variants.reserve(m_variants.size() + variants.size());
	for (SceneMaterialVariantDesc& variant : variants)
	{
		m_variants.push_back(std::move(variant));
	}

	m_bindings.reserve(m_bindings.size() + bindings.size());
	for (SceneMaterialVariantBinding& binding : bindings)
	{
		if (binding.variantIndex == kInvalidSceneMaterialVariantIndex)
		{
			continue;
		}

		binding.variantIndex += variantBaseIndex;
		m_bindings.push_back(binding);
	}
}

bool SceneMaterialVariants::ApplyVariant(SceneMaterialVariantIndex variantIndex, SceneMeshes& meshes)
{
	if (variantIndex >= m_variants.size())
	{
		return false;
	}

	bool appliedAnyBinding = false;
	for (const SceneMaterialVariantBinding& binding : m_bindings)
	{
		if (binding.variantIndex != variantIndex)
		{
			continue;
		}

		appliedAnyBinding = meshes.SetMeshMaterial(binding.meshInstanceIndex, binding.material) || appliedAnyBinding;
	}

	if (appliedAnyBinding)
	{
		m_activeVariantIndex = variantIndex;
	}
	return appliedAnyBinding;
}

void SceneMaterialVariants::Reset() noexcept
{
	m_variants.clear();
	m_bindings.clear();
	m_activeVariantIndex = kInvalidSceneMaterialVariantIndex;
}
