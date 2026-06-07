#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Mesh.h"
#include "SkeletalMeshData.h"

class SPARKLE_ENGINE_API SkeletalCookedMesh final : public Mesh
{
  public:
	SkeletalCookedMesh(SkeletalMeshData&& meshData, Assets::CookedAssetId assetId) noexcept;
	~SkeletalCookedMesh() override = default;

	SkeletalCookedMesh(const SkeletalCookedMesh&) = delete;
	SkeletalCookedMesh& operator=(const SkeletalCookedMesh&) = delete;
	SkeletalCookedMesh(SkeletalCookedMesh&&) noexcept = default;
	SkeletalCookedMesh& operator=(SkeletalCookedMesh&&) noexcept = default;

	Assets::CookedAssetId GetAssetId() const noexcept { return m_assetId; }
	const SkeletalMeshData& GetSkeletalMeshData() const noexcept { return m_skeletalData; }

  protected:
	void GenerateGeometry(MeshData& outMeshData) const override;

  private:
	SkeletalMeshData m_skeletalData;
	Assets::CookedAssetId m_assetId = Assets::InvalidCookedAssetId;
};
