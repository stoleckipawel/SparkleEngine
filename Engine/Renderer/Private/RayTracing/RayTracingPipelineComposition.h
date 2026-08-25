#pragma once

#include "RHI/Public/RayTracing/RhiRayTracingPipelineDesc.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct RayTracingHitGroupComposition final
{
	std::string ExportName;
	ERhiRayTracingHitGroupKind Kind = ERhiRayTracingHitGroupKind::Triangles;
	ShaderTypeId ClosestHit = 0;
	ShaderTypeId AnyHit = 0;
	ShaderTypeId Intersection = 0;
	std::vector<std::byte> LocalData;

	bool operator==(const RayTracingHitGroupComposition&) const noexcept = default;

	template <typename TClosestHit, typename TAnyHit = void> static RayTracingHitGroupComposition Triangles(std::string exportName)
	{
		RayTracingHitGroupComposition result;
		result.ExportName = std::move(exportName);
		result.Kind = ERhiRayTracingHitGroupKind::Triangles;
		result.ClosestHit = GlobalShader<TClosestHit>::GetRegistration().TypeId;
		if constexpr (!std::is_void_v<TAnyHit>)
		{
			result.AnyHit = GlobalShader<TAnyHit>::GetRegistration().TypeId;
		}
		return result;
	}

	template <typename TClosestHit, typename TIntersection, typename TAnyHit = void>
	static RayTracingHitGroupComposition Procedural(std::string exportName)
	{
		RayTracingHitGroupComposition result = Triangles<TClosestHit, TAnyHit>(std::move(exportName));
		result.Kind = ERhiRayTracingHitGroupKind::Procedural;
		result.Intersection = GlobalShader<TIntersection>::GetRegistration().TypeId;
		return result;
	}

	template <typename TLocalData> void SetLocalData(const TLocalData& value)
	{
		static_assert(std::is_trivially_copyable_v<TLocalData>);
		const auto bytes = std::as_bytes(std::span{&value, 1u});
		LocalData.assign(bytes.begin(), bytes.end());
	}
};

class RayTracingPipelineComposition final
{
public:
	RayTracingPipelineComposition(const RayTracingPipelineComposition&) = default;
	RayTracingPipelineComposition& operator=(const RayTracingPipelineComposition&) = default;
	RayTracingPipelineComposition(RayTracingPipelineComposition&&) noexcept = default;
	RayTracingPipelineComposition& operator=(RayTracingPipelineComposition&&) noexcept = default;

	template <typename TRayGeneration> static RayTracingPipelineComposition Create(
	    std::vector<ShaderTypeId> missShaders,
	    std::vector<RayTracingHitGroupComposition> hitGroups,
	    std::vector<ShaderTypeId> callableShaders = {})
	{
		return RayTracingPipelineComposition(
		    GlobalShader<TRayGeneration>::GetRegistration().TypeId,
		    std::move(missShaders),
		    std::move(hitGroups),
		    std::move(callableShaders));
	}

	template <typename TShader> static ShaderTypeId Shader() { return GlobalShader<TShader>::GetRegistration().TypeId; }

	ShaderTypeId GetRayGeneration() const noexcept { return m_rayGeneration; }
	std::span<const ShaderTypeId> GetMissShaders() const noexcept { return m_missShaders; }
	std::span<const RayTracingHitGroupComposition> GetHitGroups() const noexcept { return m_hitGroups; }
	std::span<const ShaderTypeId> GetCallableShaders() const noexcept { return m_callableShaders; }
	bool operator==(const RayTracingPipelineComposition&) const noexcept = default;

private:
	RayTracingPipelineComposition(
	    ShaderTypeId rayGeneration,
	    std::vector<ShaderTypeId> missShaders,
	    std::vector<RayTracingHitGroupComposition> hitGroups,
	    std::vector<ShaderTypeId> callableShaders);

	ShaderTypeId m_rayGeneration = 0;
	std::vector<ShaderTypeId> m_missShaders;
	std::vector<RayTracingHitGroupComposition> m_hitGroups;
	std::vector<ShaderTypeId> m_callableShaders;
};
