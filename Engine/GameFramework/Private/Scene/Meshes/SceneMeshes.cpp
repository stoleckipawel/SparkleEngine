#include "PCH.h"

#include "Scene/Meshes/SceneMeshes.h"

#include "Scene/Meshes/MeshComponent.h"

SceneMeshes::SceneMeshes() noexcept = default;

SceneMeshes::~SceneMeshes() noexcept = default;

void SceneMeshes::AppendMeshComponents(std::vector<std::unique_ptr<MeshComponent>>&& meshes)
{
	if (meshes.empty())
	{
		return;
	}

	m_meshes.reserve(m_meshes.size() + meshes.size());
	for (std::unique_ptr<MeshComponent>& mesh : meshes)
	{
		if (!mesh)
		{
			continue;
		}

		m_meshes.push_back(std::move(mesh));
	}
}

void SceneMeshes::Reset() noexcept
{
	m_meshes.clear();
}

MeshSnapshot SceneMeshes::CaptureSnapshot() const
{
	MeshSnapshot snapshot;
	snapshot.meshComponents.reserve(m_meshes.size());

	for (const std::unique_ptr<MeshComponent>& mesh : m_meshes)
	{
		if (!mesh || !mesh->IsVisible())
		{
			continue;
		}

		snapshot.meshComponents.push_back(mesh.get());
	}

	return snapshot;
}