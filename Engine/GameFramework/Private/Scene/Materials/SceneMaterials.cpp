#include "PCH.h"

#include "Scene/Materials/SceneMaterials.h"

void SceneMaterials::AppendMaterials(std::vector<MaterialDesc>&& materialDescs)
{
	if (materialDescs.empty())
	{
		return;
	}

	m_materialDescs.reserve(m_materialDescs.size() + materialDescs.size());
	for (MaterialDesc& materialDesc : materialDescs)
	{
		m_materialDescs.push_back(std::move(materialDesc));
	}
}

MaterialSnapshot SceneMaterials::CaptureSnapshot() const
{
	MaterialSnapshot snapshot;
	snapshot.materialDescs = m_materialDescs;
	return snapshot;
}