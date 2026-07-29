#pragma once

#include "Core/Public/Math/MathUtils.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <string>

enum class ImportedCameraProjectionKind : std::uint32_t
{
	Perspective = 0,
	Orthographic = 1,
	Unknown = 2,
};

struct ImportedCamera
{
	std::string name;
	DirectX::XMFLOAT4X4 worldTransform = MathUtils::IdentityFloat4x4();
	ImportedCameraProjectionKind projectionKind = ImportedCameraProjectionKind::Unknown;
	float fovYRadians = 0.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;
	std::uint32_t sourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();

	bool IsPerspective() const noexcept { return projectionKind == ImportedCameraProjectionKind::Perspective; }
};
