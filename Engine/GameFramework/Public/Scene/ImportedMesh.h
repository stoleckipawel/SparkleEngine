#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "Mesh.h"

#include <DirectXMath.h>

class SPARKLE_ENGINE_API ImportedMesh final : public Mesh
{
  public:
	ImportedMesh(MeshData&& meshData, const DirectX::XMFLOAT4X4& worldTransform) noexcept;

	~ImportedMesh() override = default;

	ImportedMesh(const ImportedMesh&) = delete;
	ImportedMesh& operator=(const ImportedMesh&) = delete;
	ImportedMesh(ImportedMesh&&) noexcept = default;
	ImportedMesh& operator=(ImportedMesh&&) noexcept = default;

	DirectX::XMMATRIX GetWorldMatrix() const noexcept override;
	DirectX::XMMATRIX GetWorldInverseTransposeMatrix() const noexcept override;

  protected:
	void GenerateGeometry(MeshData& outMeshData) const override;

  private:
	MeshData m_importedData;
	DirectX::XMFLOAT4X4 m_worldTransform;
};
