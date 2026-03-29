#include "PCH.h"

#include "Scene/Materials/SceneMaterials.h"

MaterialDesc SceneMaterials::CreateDefaultMaterial()
{
	MaterialDesc defaultMaterial;
	defaultMaterial.name = "Scene_DefaultMaterial";
	return defaultMaterial;
}

MaterialHandle SceneMaterials::AppendMaterials(std::vector<MaterialDesc>&& materialDescs)
{
	if (materialDescs.empty())
	{
		return GetOrCreateDefaultMaterialHandle();
	}

	const MaterialHandle materialBaseHandle(static_cast<std::uint32_t>(m_materialDescs.size()));
	m_materialDescs.reserve(m_materialDescs.size() + materialDescs.size());
	for (MaterialDesc& materialDesc : materialDescs)
	{
		m_materialDescs.push_back(std::move(materialDesc));
	}

	return materialBaseHandle;
}

MaterialHandle SceneMaterials::GetOrCreateDefaultMaterialHandle()
{
	if (!m_defaultMaterialHandle.IsValid())
	{
		m_defaultMaterialHandle = MaterialHandle(static_cast<std::uint32_t>(m_materialDescs.size()));
		m_materialDescs.push_back(CreateDefaultMaterial());
	}

	return m_defaultMaterialHandle;
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
	m_defaultMaterialHandle = MaterialHandle::Invalid();
}