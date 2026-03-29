#include "PCH.h"

#include "Scene/Materials/SceneMaterials.h"

MaterialDesc SceneMaterials::CreateDefaultMaterial()
{
	MaterialDesc defaultMaterial;
	defaultMaterial.name = "Scene_DefaultMaterial";
	return defaultMaterial;
}

std::uint32_t SceneMaterials::AppendMaterials(std::vector<MaterialDesc>&& materialDescs)
{
	if (materialDescs.empty())
	{
		return GetOrCreateDefaultMaterialId();
	}

	const std::uint32_t materialBaseId = static_cast<std::uint32_t>(m_materialDescs.size());
	m_materialDescs.reserve(m_materialDescs.size() + materialDescs.size());
	for (MaterialDesc& materialDesc : materialDescs)
	{
		m_materialDescs.push_back(std::move(materialDesc));
	}

	return materialBaseId;
}

std::uint32_t SceneMaterials::GetOrCreateDefaultMaterialId()
{
	if (!m_defaultMaterialId.has_value())
	{
		m_defaultMaterialId = static_cast<std::uint32_t>(m_materialDescs.size());
		m_materialDescs.push_back(CreateDefaultMaterial());
	}

	return *m_defaultMaterialId;
}

MaterialSnapshot SceneMaterials::CaptureSnapshot() const
{
	MaterialSnapshot snapshot;
	snapshot.materialDescs = m_materialDescs;
	return snapshot;
}

void SceneMaterials::Reset() noexcept
{
	m_materialDescs.clear();
	m_defaultMaterialId.reset();
}