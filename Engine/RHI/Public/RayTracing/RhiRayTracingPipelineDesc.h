#pragma once

#include "../Pipeline/RhiPipelineDesc.h"
#include "../Resources/RhiResourceHandles.h"
#include "../RHIAPI.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

enum class ERhiRayTracingHitGroupKind : std::uint8_t
{
	Triangles,
	Procedural,
};

struct RhiRayTracingShaderExportDesc final
{
	const ResolvedShader* Shader = nullptr;
	std::string_view ExportName;
};

struct RhiRayTracingHitGroupDesc final
{
	std::string_view ExportName;
	ERhiRayTracingHitGroupKind Kind = ERhiRayTracingHitGroupKind::Triangles;
	std::string_view ClosestHitExport;
	std::string_view AnyHitExport;
	std::string_view IntersectionExport;
};

struct RayTracingPipelineDesc final
{
	const RenderBindingLayout* GlobalBindingLayout = nullptr;
	std::span<const RhiRayTracingShaderExportDesc> ShaderExports;
	std::span<const RhiRayTracingHitGroupDesc> HitGroups;
	std::uint32_t MaxPayloadSizeInBytes = 0;
	std::uint32_t MaxAttributeSizeInBytes = 0;
	std::uint32_t MaxRecursionDepth = 1;
	std::uint64_t Generation = 0;
	const wchar_t* DebugName = L"RHI_RayTracingPipeline";
};

class SPARKLE_RHI_API RayTracingPipeline
{
public:
	struct RecordContract final
	{
		std::string ExportName;
		ShaderStage Stage = ShaderStage::Count;
		std::uint32_t LocalRecordSizeInBytes = 0;
		ShaderParameterSignature LocalRecordSignature = 0;
		bool IsHitGroup = false;
	};

	explicit RayTracingPipeline(const RayTracingPipelineDesc& desc);
	virtual ~RayTracingPipeline() noexcept;

	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	const RecordContract* FindRecordContract(std::string_view exportName) const noexcept;

private:
	std::uint64_t m_generation = 0;
	std::vector<RecordContract> m_recordContracts;
};

struct RhiRayTracingShaderRecord final
{
	std::string_view ExportName;
	std::span<const std::byte> LocalData;
	ShaderParameterSignature LocalRecordSignature = 0;
};

struct RayTracingShaderTableDesc final
{
	const RayTracingPipeline* Pipeline = nullptr;
	std::span<const RhiRayTracingShaderRecord> RayGenerationRecords;
	std::span<const RhiRayTracingShaderRecord> MissRecords;
	std::span<const RhiRayTracingShaderRecord> HitGroupRecords;
	std::span<const RhiRayTracingShaderRecord> CallableRecords;
	std::uint64_t Generation = 0;
	const wchar_t* DebugName = L"RHI_RayTracingShaderTable";
};

struct RhiRayTracingShaderTableRegion;

class SPARKLE_RHI_API RayTracingShaderTable
{
public:
	RayTracingShaderTable(std::uint64_t generation, std::uint64_t pipelineGeneration) noexcept;
	virtual ~RayTracingShaderTable() noexcept;

	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	std::uint64_t GetPipelineGeneration() const noexcept { return m_pipelineGeneration; }
	virtual RhiResourceHandle GetResource() const noexcept = 0;
	virtual RhiRayTracingShaderTableRegion GetRayGenerationRegion() const noexcept = 0;
	virtual RhiRayTracingShaderTableRegion GetMissRegion() const noexcept = 0;
	virtual RhiRayTracingShaderTableRegion GetHitGroupRegion() const noexcept = 0;
	virtual RhiRayTracingShaderTableRegion GetCallableRegion() const noexcept = 0;

private:
	std::uint64_t m_generation = 0;
	std::uint64_t m_pipelineGeneration = 0;
};

struct RhiRayTracingShaderTableRegion final
{
	std::uint64_t OffsetInBytes = 0;
	std::uint64_t SizeInBytes = 0;
	std::uint64_t StrideInBytes = 0;

	constexpr bool operator==(const RhiRayTracingShaderTableRegion&) const noexcept = default;
};

struct TraceRaysDesc final
{
	const RayTracingPipeline* Pipeline = nullptr;
	const RayTracingShaderTable* ShaderTable = nullptr;
	RhiRayTracingShaderTableRegion RayGeneration;
	RhiRayTracingShaderTableRegion Miss;
	RhiRayTracingShaderTableRegion HitGroup;
	RhiRayTracingShaderTableRegion Callable;
	std::uint32_t Width = 0;
	std::uint32_t Height = 0;
	std::uint32_t Depth = 1;
};
