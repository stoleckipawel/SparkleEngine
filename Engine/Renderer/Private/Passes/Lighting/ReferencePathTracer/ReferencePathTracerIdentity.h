#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"

#include <array>
#include <cstddef>
#include <cstdint>

struct PreparedRenderScene;
struct RenderFrameIdentity;
struct RenderView;

enum class ReferencePathTracerIdentityComponent : std::uint8_t
{
	None = 0,
	View,
	Camera,
	Geometry,
	Deformation,
	Material,
	Light,
	Environment,
	Shader,
	Backend,
};

struct ReferencePathTracerIdentity final
{
	static constexpr std::size_t ComponentCount = 9u;

	std::array<std::uint64_t, ComponentCount> Components = {};

	bool operator==(const ReferencePathTracerIdentity&) const noexcept = default;
	ReferencePathTracerIdentityComponent FindFirstDifference(const ReferencePathTracerIdentity& other) const noexcept;
};

ReferencePathTracerIdentity BuildReferencePathTracerIdentity(
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration,
    ERhiBackendApi backendApi) noexcept;
