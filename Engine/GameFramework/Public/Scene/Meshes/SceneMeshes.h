#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"

#include <cstddef>
#include <memory>
#include <vector>

class Mesh;

class SPARKLE_ENGINE_API SceneMeshes final
{
  public:
	SceneMeshes() noexcept;
	~SceneMeshes() noexcept;

	SceneMeshes(const SceneMeshes&) = delete;
	SceneMeshes& operator=(const SceneMeshes&) = delete;
	SceneMeshes(SceneMeshes&&) = delete;
	SceneMeshes& operator=(SceneMeshes&&) = delete;

	std::size_t GetMeshCount() const noexcept { return m_meshes.size(); }
	bool HasMeshes() const noexcept { return !m_meshes.empty(); }

	const Mesh* GetMesh(std::size_t index) const noexcept { return m_meshes[index].get(); }
	Mesh* GetMesh(std::size_t index) noexcept { return m_meshes[index].get(); }

	void AppendMeshes(std::vector<std::unique_ptr<Mesh>>&& meshes);
	MeshSnapshot CaptureSnapshot() const;
	void Reset() noexcept;

  private:
	std::vector<std::unique_ptr<Mesh>> m_meshes;
};