#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"

#include <array>
#include <cstddef>
#include <cstdint>

struct PreparedRenderScene;
struct RenderFrameIdentity;
struct RenderView;

enum class ReferencePathTracerResetReason : std::uint8_t
{
	None,
	View,
	Camera,
	Geometry,
	Deformation,
	Material,
	Light,
	Environment,
	Shader,
	Transport,
	Restart,
};

struct ReferencePathTracerIdentity final
{
	static constexpr std::size_t ComponentCount = 9u;

	std::array<std::uint64_t, ComponentCount> Components = {};

	bool operator==(const ReferencePathTracerIdentity&) const noexcept = default;
};

ReferencePathTracerIdentity BuildReferencePathTracerIdentity(
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration,
    ERhiBackendApi backendApi) noexcept;

ReferencePathTracerResetReason ClassifyReferencePathTracerIdentityChange(
    const ReferencePathTracerIdentity& current,
    const ReferencePathTracerIdentity& next) noexcept;
