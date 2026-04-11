#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Mesh.h"

class SPARKLE_ENGINE_API CookedMesh final : public Mesh
{
  public:
	explicit CookedMesh(MeshData&& meshData) noexcept;

	~CookedMesh() override = default;

	CookedMesh(const CookedMesh&) = delete;
	CookedMesh& operator=(const CookedMesh&) = delete;
	CookedMesh(CookedMesh&&) noexcept = default;
	CookedMesh& operator=(CookedMesh&&) noexcept = default;

  protected:
	void GenerateGeometry(MeshData& outMeshData) const override;

  private:
	MeshData m_cookedData;
};