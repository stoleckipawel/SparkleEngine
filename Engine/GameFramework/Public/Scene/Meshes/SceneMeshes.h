#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Meshes/MeshSnapshot.h"

#include <cstddef>
#include <memory>
#include <vector>

class MeshComponent;

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
	std::size_t GetMeshInstanceGroupCount() const noexcept { return m_meshInstanceGroups.size(); }
	bool HasMeshes() const noexcept { return !m_meshes.empty(); }

	const MeshComponent* GetMeshComponent(std::size_t index) const noexcept { return m_meshes[index].get(); }
	MeshComponent* GetMeshComponent(std::size_t index) noexcept { return m_meshes[index].get(); }

	void AppendMeshComponents(std::vector<std::unique_ptr<MeshComponent>>&& meshes);
	void AppendMeshInstanceGroups(std::vector<MeshInstanceGroupSnapshot>&& meshInstanceGroups);
	MeshSnapshot CaptureSnapshot() const;
	void Reset() noexcept;

  private:
	std::vector<std::unique_ptr<MeshComponent>> m_meshes;
	std::vector<MeshInstanceGroupSnapshot> m_meshInstanceGroups;
};