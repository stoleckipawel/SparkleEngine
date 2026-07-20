#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Mesh.h"
#include "SkeletalMeshData.h"

#include <span>
#include <vector>

class SPARKLE_ENGINE_API SkeletalCookedMesh final : public Mesh
{
  public:
	SkeletalCookedMesh(SkeletalMeshData&& meshData, Assets::CookedAssetId assetId, std::span<const float> initialMorphWeights = {}) noexcept;
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
	MeshData m_baseGeometry;
	Assets::CookedAssetId m_assetId = Assets::InvalidCookedAssetId;
};
