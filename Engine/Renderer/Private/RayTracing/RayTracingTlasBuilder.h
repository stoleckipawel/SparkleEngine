#pragma once

#include "RayTracing/RayTracingBlasCache.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <vector>

class RenderCommandContext;
struct RenderSceneData;

class RayTracingTlasBuilder final
{
  public:
	struct BuildStats final
	{
		std::uint32_t instanceCount = 0;
		bool builtTlas = false;
	};

	struct TlasHandle final
	{
		NativeResourceHandle resource = {};
		RhiGpuVirtualAddress gpuAddress = 0;
		std::uint32_t instanceCount = 0;

		bool IsValid() const noexcept { return resource && gpuAddress != 0 && instanceCount > 0; }
	};

	explicit RayTracingTlasBuilder(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~RayTracingTlasBuilder() noexcept;

	RayTracingTlasBuilder(const RayTracingTlasBuilder&) = delete;
	RayTracingTlasBuilder& operator=(const RayTracingTlasBuilder&) = delete;
	RayTracingTlasBuilder(RayTracingTlasBuilder&&) = delete;
	RayTracingTlasBuilder& operator=(RayTracingTlasBuilder&&) = delete;

	BuildStats Build(RenderCommandContext& cmd, const RenderSceneData& sceneData, RayTracingBlasCache& blasCache) noexcept;
	const TlasHandle& GetTlas() const noexcept { return m_tlas; }
	void Clear() noexcept;

  private:
	static std::array<float, 12> BuildInstanceTransform(const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
	void ReleaseResources() noexcept;
	bool EnsureResources(const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo) noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_instanceBuffer = {};
	RhiOwnedResourceHandle m_scratchBuffer = {};
	RhiOwnedResourceHandle m_accelerationStructureBuffer = {};
	std::uint64_t m_scratchBufferSizeInBytes = 0;
	std::uint64_t m_accelerationStructureSizeInBytes = 0;
	TlasHandle m_tlas = {};
};
