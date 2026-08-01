#pragma once

#include "Types/ImportedGeometry.h"

#include <mikktspace.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

class GltfMikkTangentContext final
{
  public:
	GltfMikkTangentContext(std::span<const ImportedVertex> vertices, std::span<const std::uint32_t> indices);

	std::vector<DirectX::XMFLOAT4> Generate();

  private:
	static constexpr std::size_t kVerticesPerTriangle = 3u;
	static constexpr float kMinimumVectorLengthSquared = 1.0e-12f;
	static constexpr float kFrameTolerance = 1.0e-3f;

	static bool IsFinite(const DirectX::XMFLOAT2& value) noexcept;
	static bool IsFinite(const DirectX::XMFLOAT3& value) noexcept;
	static bool IsUsableTangent(const DirectX::XMFLOAT4& tangent, const DirectX::XMFLOAT3& normal) noexcept;

	void ValidateSourceGeometry() const;
	void ValidateTriangle(std::size_t faceIndex) const;
	void ValidateTangentDerivatives(std::size_t faceIndex) const;
	const ImportedVertex& GetVertex(int faceIndex, int faceVertexIndex) const noexcept;

	static GltfMikkTangentContext& GetOwner(const SMikkTSpaceContext* context) noexcept;
	static int GetFaceCount(const SMikkTSpaceContext* context);
	static int GetFaceVertexCount(const SMikkTSpaceContext* context, int faceIndex);
	static void GetPosition(const SMikkTSpaceContext* context, float output[], int faceIndex, int faceVertexIndex);
	static void GetNormal(const SMikkTSpaceContext* context, float output[], int faceIndex, int faceVertexIndex);
	static void GetTextureCoordinate(const SMikkTSpaceContext* context, float output[], int faceIndex, int faceVertexIndex);
	static void SetTangent(const SMikkTSpaceContext* context, const float tangent[], float sign, int faceIndex, int faceVertexIndex);
	static SMikkTSpaceInterface BuildInterface() noexcept;

	std::span<const ImportedVertex> m_vertices;
	std::span<const std::uint32_t> m_indices;
	std::vector<DirectX::XMFLOAT4> m_cornerTangents;
};
