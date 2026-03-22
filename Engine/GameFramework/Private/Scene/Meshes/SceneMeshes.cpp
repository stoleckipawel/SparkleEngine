#include "PCH.h"

#include "Scene/Meshes/SceneMeshes.h"

#include "Scene/Mesh.h"

SceneMeshes::SceneMeshes() noexcept = default;

SceneMeshes::~SceneMeshes() noexcept = default;

void SceneMeshes::AppendMeshes(std::vector<std::unique_ptr<Mesh>>&& meshes)
{
	if (meshes.empty())
	{
		return;
	}

	m_meshes.reserve(m_meshes.size() + meshes.size());
	for (std::unique_ptr<Mesh>& mesh : meshes)
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
	snapshot.meshPointers.reserve(m_meshes.size());

	for (const std::unique_ptr<Mesh>& mesh : m_meshes)
	{
		if (!mesh)
		{
			continue;
		}

		snapshot.meshPointers.push_back(mesh.get());
	}

	return snapshot;
}