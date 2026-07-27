#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "MeshData.h"

class SPARKLE_ENGINE_API Mesh
{
  public:
	virtual ~Mesh();
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept;
	Mesh& operator=(Mesh&&) noexcept;

	const MeshData& GetMeshData() const noexcept;

  protected:
	explicit Mesh(MeshData&& meshData) noexcept;
	explicit Mesh(const MeshData& meshData);

  private:
	MeshData m_meshData;
};
