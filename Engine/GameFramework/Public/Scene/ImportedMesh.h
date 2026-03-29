#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Mesh.h"

class SPARKLE_ENGINE_API ImportedMesh final : public Mesh
{
  public:
	explicit ImportedMesh(MeshData&& meshData) noexcept;

	~ImportedMesh() override = default;

	ImportedMesh(const ImportedMesh&) = delete;
	ImportedMesh& operator=(const ImportedMesh&) = delete;
	ImportedMesh(ImportedMesh&&) noexcept = default;
	ImportedMesh& operator=(ImportedMesh&&) noexcept = default;

  protected:
	void GenerateGeometry(MeshData& outMeshData) const override;

  private:
	MeshData m_importedData;
};
