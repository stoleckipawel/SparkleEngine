#include "PCH.h"

#include "RHI/Public/Validation/RhiValidation.h"

#include <format>

bool RhiValidation::ValidateRayTracingGeometryDesc(
    const RhiRayTracingGeometryDesc& geometry,
    std::string_view owner) noexcept
{
	if (geometry.VertexBuffer == 0 || geometry.IndexBuffer == 0)
	{
		ReportContractViolation(owner, "ray tracing geometry has a null vertex or index GPU address", "upload mesh buffers before AS build");
		return false;
	}
	if (geometry.VertexStrideInBytes < sizeof(float) * 3u)
	{
		ReportContractViolation(
		    owner,
		    "ray tracing geometry vertex stride is smaller than float3 position data",
		    "provide a vertex buffer layout whose first attribute is float3 position");
		return false;
	}
	if (geometry.VertexCount == 0 || geometry.IndexCount == 0)
	{
		ReportContractViolation(owner, "ray tracing geometry has zero vertices or indices", "skip empty meshes before AS build");
		return false;
	}
	if (geometry.IndexCount % 3u != 0)
	{
		ReportContractViolation(
		    owner,
		    "ray tracing geometry index count is not divisible by three",
		    "build BLAS only from triangle-list geometry");
		return false;
	}
	return true;
}

bool RhiValidation::ValidateRayTracingInstanceDescs(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::string_view owner) noexcept
{
	if (instances == nullptr || instanceCount == 0)
	{
		ReportContractViolation(owner, "ray tracing instance descriptor list is empty", "skip TLAS build until at least one BLAS instance exists");
		return false;
	}

	for (std::uint32_t index = 0; index < instanceCount; ++index)
	{
		const RhiRayTracingInstanceDesc& instance = instances[index];
		if (instance.AccelerationStructure == 0)
		{
			const std::string condition = std::format("ray tracing instance {} references a null BLAS address", index);
			ReportContractViolation(owner, condition, "build the BLAS and provide its GPU address before adding the TLAS instance");
			return false;
		}
		if (instance.InstanceMask == 0)
		{
			const std::string condition = std::format("ray tracing instance {} has a zero instance mask", index);
			ReportContractViolation(owner, condition, "use a nonzero instance mask so TraceRay/RayQuery visibility can hit the instance");
			return false;
		}
	}

	return true;
}

bool RhiValidation::ValidateRayTracingAccelerationStructurePrebuildInfo(
    const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo,
    std::string_view owner) noexcept
{
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
	{
		ReportContractViolation(
		    owner,
		    "ray tracing AS prebuild info returned a zero result or scratch size",
		    "verify backend ray tracing capability and AS build input descriptors before allocating AS resources");
		return false;
	}
	return true;
}

bool RhiValidation::ValidateRayTracingScratchOrResultAddress(
    RhiGpuVirtualAddress gpuAddress,
    std::string_view addressRole,
    std::string_view owner) noexcept
{
	if (gpuAddress == 0)
	{
		const std::string condition = std::format("ray tracing {} GPU address is null", addressRole);
		ReportContractViolation(owner, condition, "allocate AS scratch/result buffers and query their GPU virtual addresses before build");
		return false;
	}
	return true;
}

bool RhiValidation::ValidateRayTracingBufferSize(
    std::uint64_t sizeInBytes,
    std::uint64_t alignmentInBytes,
    std::string_view owner) noexcept
{
	if (sizeInBytes == 0)
	{
		ReportContractViolation(owner, "ray tracing buffer size is zero", "use AS prebuild info to size ray tracing buffers");
		return false;
	}
	if (alignmentInBytes != 0 && sizeInBytes % alignmentInBytes != 0)
	{
		const std::string condition =
		    std::format("ray tracing buffer size {} is not aligned to {}", sizeInBytes, alignmentInBytes);
		ReportContractViolation(owner, condition, "round AS buffers up to the backend-reported ray tracing alignment");
		return false;
	}
	return true;
}
