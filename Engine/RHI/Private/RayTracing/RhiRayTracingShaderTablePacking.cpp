#include "PCH.h"

#include "RayTracing/RhiRayTracingShaderTablePacking.h"

#include "Core/Public/Diagnostics/Error.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace RhiRayTracingShaderTablePacking
{
	std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment)
	{
		if (alignment == 0 || value > std::numeric_limits<std::uint64_t>::max() - (alignment - 1))
		{
			throw Diagnostics::Error("Ray-tracing shader-table alignment overflowed.");
		}
		return (value + alignment - 1) / alignment * alignment;
	}

	RhiRayTracingShaderTableRegion AppendRegion(
	    std::span<const RhiRayTracingShaderRecord> records,
	    std::span<const std::byte> identifiers,
	    const RhiRayTracingShaderTablePackingRules& rules,
	    std::vector<std::byte>& bytes)
	{
		if (records.empty())
		{
			if (!identifiers.empty())
			{
				throw Diagnostics::Error("An empty ray-tracing shader-table region has identifier data.");
			}
			return {};
		}
		if (rules.IdentifierSizeInBytes == 0
		    || records.size() > std::numeric_limits<std::uint64_t>::max() / rules.IdentifierSizeInBytes
		    || records.size() * rules.IdentifierSizeInBytes != identifiers.size())
		{
			throw Diagnostics::Error("Ray-tracing shader-table identifiers do not match the record count.");
		}

		std::size_t maximumLocalDataSize = 0;
		for (const RhiRayTracingShaderRecord& record : records)
		{
			maximumLocalDataSize = std::max(maximumLocalDataSize, record.LocalData.size());
		}
		if (maximumLocalDataSize > std::numeric_limits<std::uint64_t>::max() - rules.IdentifierSizeInBytes)
		{
			throw Diagnostics::Error("Ray-tracing shader-table record size overflowed.");
		}
		const std::uint64_t unalignedStride = rules.IdentifierSizeInBytes + static_cast<std::uint64_t>(maximumLocalDataSize);
		const std::uint64_t stride = AlignUp(unalignedStride, rules.RecordAlignmentInBytes);
		if (stride > rules.MaximumRecordStrideInBytes)
		{
			throw Diagnostics::Error("Ray-tracing shader-table record exceeds the backend stride limit.");
		}
		const std::uint64_t offset = AlignUp(bytes.size(), rules.TableAlignmentInBytes);
		if (records.size() > std::numeric_limits<std::uint64_t>::max() / stride)
		{
			throw Diagnostics::Error("Ray-tracing shader-table size arithmetic overflowed.");
		}
		const std::uint64_t size = stride * records.size();
		if (offset > std::numeric_limits<std::uint64_t>::max() - size || offset + size > std::numeric_limits<std::size_t>::max())
		{
			throw Diagnostics::Error("Ray-tracing shader-table size arithmetic overflowed.");
		}

		bytes.resize(static_cast<std::size_t>(offset + size));
		for (std::size_t index = 0; index < records.size(); ++index)
		{
			const RhiRayTracingShaderRecord& record = records[index];
			std::byte* const destination = bytes.data() + static_cast<std::size_t>(offset + stride * index);
			const std::byte* const identifier = identifiers.data() + index * rules.IdentifierSizeInBytes;
			std::memcpy(destination, identifier, static_cast<std::size_t>(rules.IdentifierSizeInBytes));
			if (!record.LocalData.empty())
			{
				std::memcpy(destination + rules.IdentifierSizeInBytes, record.LocalData.data(), record.LocalData.size());
			}
		}
		return RhiRayTracingShaderTableRegion{.OffsetInBytes = offset, .SizeInBytes = size, .StrideInBytes = stride};
	}
}
