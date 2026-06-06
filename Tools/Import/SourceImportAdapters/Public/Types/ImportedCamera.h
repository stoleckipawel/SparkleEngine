#pragma once

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
	DirectX::XMFLOAT4X4 worldTransform = {
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    0.0f,
	    1.0f};
	ImportedCameraProjectionKind projectionKind = ImportedCameraProjectionKind::Unknown;
	float verticalFovRadians = 0.0f;
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;
	std::uint32_t sourceNodeIndex = (std::numeric_limits<std::uint32_t>::max)();

	bool IsPerspective() const noexcept { return projectionKind == ImportedCameraProjectionKind::Perspective; }
};
