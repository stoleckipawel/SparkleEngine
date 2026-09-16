#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RayTracing/Effects/RayTracingExecutionFrontend.h"

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
	Execution,
	Backend,
};

struct ReferencePathTracerIdentity final
{
	static constexpr std::size_t ComponentCount = 10u;

	std::array<std::uint64_t, ComponentCount> Components = {};

	bool operator==(const ReferencePathTracerIdentity&) const noexcept = default;
	ReferencePathTracerIdentityComponent FindFirstDifference(const ReferencePathTracerIdentity& other) const noexcept;
};

ReferencePathTracerIdentity BuildReferencePathTracerIdentity(
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration,
    RayTracingExecutionFrontend executionFrontend,
    ERhiBackendApi backendApi) noexcept;
