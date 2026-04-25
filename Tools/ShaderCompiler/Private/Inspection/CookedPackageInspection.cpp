#include "PCH.h"

#include "Inspection/CookedPackageInspection.h"

#include "Core/Public/Files/FileUtils.h"

#include <cstring>
#include <span>

namespace
{
	template <typename T>
	std::span<const T> ReadPackageArray(
	    std::span<const std::uint8_t> bytes,
	    std::size_t& cursor,
	    std::uint32_t count,
	    std::string& outErrorMessage)
	{
		static_assert(std::is_trivially_copyable_v<T>, "Cooked package inspection requires trivially copyable arrays.");

		const std::size_t byteCount = sizeof(T) * static_cast<std::size_t>(count);
		if (cursor + byteCount > bytes.size())
		{
			outErrorMessage = "Cooked shader package arrays are truncated";
			cursor = bytes.size() + 1;
			return {};
		}

		const auto* data = reinterpret_cast<const T*>(bytes.data() + cursor);
		cursor += byteCount;
		return std::span<const T>(data, count);
	}

	std::string_view ResolveString(CookedShaderStringRef ref, std::span<const std::uint8_t> stringTable) noexcept
	{
		if (!ref || ref.OffsetInBytes + ref.SizeInBytes > stringTable.size())
		{
			return {};
		}

		return std::string_view(reinterpret_cast<const char*>(stringTable.data() + ref.OffsetInBytes), ref.SizeInBytes);
	}
}

bool CookedPackageInspection::Inspect(
    const std::filesystem::path& packagePath,
    InspectedCookedShaderPackage& outPackage,
    std::string& outErrorMessage)
{
	outPackage = {};

	std::vector<std::uint8_t> bytes;
	if (!Engine::Files::TryReadAllBytes(packagePath, bytes, outErrorMessage))
	{
		return false;
	}

	if (bytes.size() < sizeof(CookedShaderPackageHeader))
	{
		outErrorMessage = "Cooked shader package is too small";
		return false;
	}

	CookedShaderPackageHeader header;
	std::memcpy(&header, bytes.data(), sizeof(header));
	if (!header.Matches(kCookedShaderPackageMagic, kCookedShaderPackageVersion))
	{
		outErrorMessage = "Unsupported cooked shader package header";
		return false;
	}

	std::size_t cursor = sizeof(CookedShaderPackageHeader);
	const auto binaries = ReadPackageArray<CookedShaderBinaryRecord>(bytes, cursor, header.BinaryRecordCount, outErrorMessage);
	ReadPackageArray<CookedShaderBindingRecord>(bytes, cursor, header.BindingRecordCount, outErrorMessage);
	ReadPackageArray<CookedShaderSpecializationInputRecord>(bytes, cursor, header.SpecializationInputCount, outErrorMessage);
	const auto reflections = ReadPackageArray<CookedShaderReflectionRecord>(bytes, cursor, header.ReflectionRecordCount, outErrorMessage);
	ReadPackageArray<CookedShaderResourceBindingRecord>(bytes, cursor, header.ResourceBindingRecordCount, outErrorMessage);
	ReadPackageArray<CookedShaderConstantBufferRecord>(bytes, cursor, header.ConstantBufferRecordCount, outErrorMessage);
	ReadPackageArray<CookedShaderConstantBufferMemberRecord>(bytes, cursor, header.ConstantBufferMemberRecordCount, outErrorMessage);
	ReadPackageArray<CookedShaderInputElementRecord>(bytes, cursor, header.InputElementRecordCount, outErrorMessage);
	ReadPackageArray<CookedShaderPushConstantRangeRecord>(bytes, cursor, header.PushConstantRangeRecordCount, outErrorMessage);
	ReadPackageArray<CookedShaderSpecializationConstantRecord>(bytes, cursor, header.SpecializationConstantRecordCount, outErrorMessage);
	if (!outErrorMessage.empty())
	{
		return false;
	}

	if (cursor + header.StringTableSizeInBytes > bytes.size())
	{
		outErrorMessage = "Cooked shader package string table is truncated";
		return false;
	}

	const std::span<const std::uint8_t> stringTable(bytes.data() + cursor, header.StringTableSizeInBytes);
	outPackage.packageKey = header.ShaderPackageKey;
	outPackage.binaryRecordCount = header.BinaryRecordCount;
	outPackage.reflectionRecordCount = header.ReflectionRecordCount;
	outPackage.binaries.reserve(binaries.size());

	for (std::size_t index = 0; index < binaries.size(); ++index)
	{
		const CookedShaderBinaryRecord& binary = binaries[index];
		InspectedCookedShaderBinary inspectedBinary{};
		inspectedBinary.stage = binary.Stage;
		inspectedBinary.format = binary.Format;
		inspectedBinary.entryPoint = std::string(ResolveString(binary.EntryPoint, stringTable));
		inspectedBinary.backendName = std::string(ResolveString(binary.BackendName, stringTable));
		inspectedBinary.bytecodeSizeInBytes = binary.Bytecode.SizeInBytes;
		if (index < reflections.size())
		{
			const CookedShaderReflectionRecord& reflection = reflections[index];
			inspectedBinary.resourceBindingCount = reflection.ResourceBindingCount;
			inspectedBinary.constantBufferCount = reflection.ConstantBufferCount;
			inspectedBinary.inputElementCount = reflection.InputElementCount;
			inspectedBinary.pushConstantRangeCount = reflection.PushConstantRangeCount;
			inspectedBinary.specializationConstantCount = reflection.SpecializationConstantCount;
		}
		outPackage.binaries.push_back(std::move(inspectedBinary));
	}

	outErrorMessage.clear();
	return true;
}

const char* CookedPackageInspection::GetBinaryFormatName(CookedShaderBinaryFormat format) noexcept
{
	switch (format)
	{
		case CookedShaderBinaryFormat::Dxil:
			return "dxil";
		case CookedShaderBinaryFormat::SpirV:
			return "spirv";
		default:
			return "unknown";
	}
}