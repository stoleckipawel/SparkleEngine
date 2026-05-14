#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Mesh.h"

class SPARKLE_ENGINE_API CookedMesh final : public Mesh
{
  public:
	explicit CookedMesh(MeshData&& meshData) noexcept;
	CookedMesh(MeshData&& meshData, Assets::CookedAssetId assetId) noexcept;

	~CookedMesh() override = default;

	CookedMesh(const CookedMesh&) = delete;
	CookedMesh& operator=(const CookedMesh&) = delete;
	CookedMesh(CookedMesh&&) noexcept = default;
	CookedMesh& operator=(CookedMesh&&) noexcept = default;

	Assets::CookedAssetId GetAssetId() const noexcept { return m_assetId; }

  protected:
	void GenerateGeometry(MeshData& outMeshData) const override;

  private:
	MeshData m_cookedData;
	Assets::CookedAssetId m_assetId = Assets::InvalidCookedAssetId;
};