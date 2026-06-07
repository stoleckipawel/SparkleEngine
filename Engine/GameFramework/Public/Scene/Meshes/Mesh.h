#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "MeshData.h"

#include <cstdint>

class SPARKLE_ENGINE_API Mesh
{
  public:
	virtual ~Mesh() = default;
	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&&) noexcept = default;
	Mesh& operator=(Mesh&&) noexcept = default;

	void RebuildGeometry();
	void MarkGeometryDirty() noexcept;
	const MeshData& GetMeshData() const;
	std::uint64_t GetGeometryRevision() const noexcept { return m_geometryRevision; }

  protected:
	Mesh() noexcept = default;
	virtual void GenerateGeometry(MeshData& outMeshData) const = 0;

  private:
	mutable MeshData m_meshData;
	mutable bool m_bGeometryDirty = true;
	std::uint64_t m_geometryRevision = 0;
};
