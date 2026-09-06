#include "PCH.h"

#include "Cooking/ShaderArtifactPublication.h"

#include "Cooking/Dependencies/ShaderDependencyManifest.h"
#include "Cooking/ReflectionSerializer.h"
#include "Cooking/ShaderRecookSignal.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringTableBuilder.h"
#include "RHI/Public/Shaders/GlobalShaderMap.h"

#include <algorithm>
#include <fstream>
#include <format>
#include <map>

namespace ShaderArtifactAssembly
{
	struct Binding final
	{
		std::string Name;
		ShaderParameterSemanticKind SemanticKind = ShaderParameterSemanticKind::ReadTexture;
		ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::None;
		ShaderParameterAccess Access = ShaderParameterAccess::None;
		ShaderStageMask VisibilityMask = ShaderStageMask::None;
		std::uint32_t LogicalBindingIndex = 0;
		std::uint32_t ArrayCount = 1;
		std::uint32_t ValueSizeInBytes = 0;
	};

	struct Entry final
	{
		ShaderTypeId ShaderType = 0;
		ShaderTarget Target = kDefaultShaderTarget;
		ShaderStage Stage = ShaderStage::Count;
		ShaderFeatureFlags Features = ShaderFeatureFlags::None;
		RayTracingShaderMetadata RayTracing;
		ShaderCodeHash CodeHash = 0;
		ShaderParameterSignature ParameterSignature = 0;
		std::uint64_t CompileInputHash = 0;
		std::uint64_t BackendVersion = 0;
		std::string ShaderName;
		std::string EntryPoint;
		std::string BackendName;
		std::string CodegenTarget;
		std::vector<Binding> Bindings;
		ShaderReflection Reflection;
		std::vector<std::uint8_t> Code;
	};

	ShaderStageMask ToStageMask(ShaderStageVisibility visibility) noexcept
	{
		switch (visibility)
		{
			case ShaderStageVisibility::Vertex:
				return ShaderStageMask::Vertex;
			case ShaderStageVisibility::Pixel:
				return ShaderStageMask::Pixel;
			case ShaderStageVisibility::Compute:
				return ShaderStageMask::Compute;
			case ShaderStageVisibility::RayTracing:
				return ShaderStageMask::AllRayTracing;
			case ShaderStageVisibility::AllGraphics:
				return ShaderStageMask::Vertex | ShaderStageMask::Pixel;
			case ShaderStageVisibility::All:
				return ShaderStageMask::Vertex | ShaderStageMask::Pixel | ShaderStageMask::Compute | ShaderStageMask::AllRayTracing;
			case ShaderStageVisibility::None:
			default:
				return ShaderStageMask::None;
		}
	}

	std::uint32_t FindBindingIndex(const ShaderCookProduct& product, std::string_view name)
	{
		const auto found =
		    std::ranges::find_if(product.bindingRemaps, [name](const ShaderDescriptorBindingRemap& remap) { return remap.Name == name; });
		if (found == product.bindingRemaps.end())
		{
			throw Diagnostics::Error(std::format("Compiled shader output is missing descriptor binding '{}'.", name));
		}
		return found->Binding;
	}

	Entry FromProduct(const ShaderCookDesc& shader, const ShaderCookProduct& product)
	{
		if (product.compiled.bytecode.empty())
		{
			throw Diagnostics::Error("A compiled shader product cannot publish empty bytecode.");
		}
		const ShaderCodeHash codeHash = Hash::Fnv1a64(product.compiled.bytecode.data(), product.compiled.bytecode.size());
		if (product.shaderTypeId != shader.shaderTypeId || !IsShaderTarget(product.target) || product.features != shader.features
		    || product.rayTracing != shader.rayTracing || product.compiled.stage != shader.stage
		    || product.compiled.format != GetShaderBinaryFormat(product.target) || product.compiled.sourcePath != shader.sourcePath
		    || product.compiled.entryPoint != shader.entryPoint || product.compiled.backendName.empty()
		    || product.compiled.codegenTarget != GetShaderTargetName(product.target) || product.compiled.backendVersion == 0
		    || product.compiled.compileInputHash == 0 || product.compiled.bytecodeHash != codeHash
		    || BuildShaderParameterSignature(product.parameterLayout) != BuildShaderParameterSignature(shader.parameterLayout))
		{
			throw Diagnostics::Error(
			    std::format("Compiled shader product for '{}' does not match its catalog contract.", shader.shaderTypeName));
		}

		Entry entry;
		entry.ShaderType = product.shaderTypeId;
		entry.Target = product.target;
		entry.Stage = product.compiled.stage;
		entry.Features = product.features;
		entry.RayTracing = product.rayTracing;
		entry.CodeHash = codeHash;
		entry.ParameterSignature = BuildShaderParameterSignature(product.parameterLayout);
		entry.CompileInputHash = product.compiled.compileInputHash;
		entry.BackendVersion = product.compiled.backendVersion;
		entry.ShaderName = shader.shaderTypeName;
		entry.EntryPoint = product.compiled.entryPoint;
		entry.BackendName = product.compiled.backendName;
		entry.CodegenTarget = product.compiled.codegenTarget;
		entry.Reflection = product.compiled.reflection;
		entry.Code = product.compiled.bytecode;
		for (const PassParameterDesc& parameter : product.parameterLayout.GetParameters())
		{
			entry.Bindings.push_back(
			    Binding{
			        .Name = parameter.Name,
			        .SemanticKind = parameter.Kind,
			        .ResourceDomain = parameter.ResourceDomain,
			        .Access = parameter.Access,
			        .VisibilityMask = ToStageMask(parameter.Visibility),
			        .LogicalBindingIndex = FindBindingIndex(product, parameter.Name),
			        .ArrayCount = parameter.ArrayCount,
			        .ValueSizeInBytes = parameter.ValueSizeInBytes});
		}
		return entry;
	}

	std::string Resolve(const GlobalShaderMap& map, std::uint32_t offset, std::uint32_t size)
	{
		return std::string(map.ResolveString(ShaderMapStringRef{offset, size}));
	}

	ShaderReflection DecodeReflection(const GlobalShaderMap& map, const GlobalShaderMapEntry& entry)
	{
		ShaderReflection result;
		const CookedShaderReflectionRecord& record = map.GetReflection(entry);
		const auto allConstantBuffers = map.GetConstantBuffers();
		const auto allMembers = map.GetConstantBufferMembers();
		for (std::uint32_t index = 0; index < record.ConstantBufferCount; ++index)
		{
			const CookedShaderConstantBufferRecord& buffer = allConstantBuffers[record.ConstantBufferOffset + index];
			ShaderReflectionConstantBuffer decoded;
			decoded.Name = Resolve(map, buffer.NameOffsetInBytes, buffer.NameSizeInBytes);
			decoded.SizeInBytes = buffer.SizeInBytes;
			for (std::uint32_t memberIndex = 0; memberIndex < buffer.MemberCount; ++memberIndex)
			{
				const CookedShaderConstantBufferMemberRecord& member = allMembers[buffer.MemberOffset + memberIndex];
				decoded.Members.push_back(
				    ShaderReflectionConstantBufferMember{
				        .Name = Resolve(map, member.NameOffsetInBytes, member.NameSizeInBytes),
				        .OffsetInBytes = member.OffsetInBytes,
				        .SizeInBytes = member.SizeInBytes,
				        .ArrayCount = member.ArrayCount,
				        .ArrayStrideInBytes = member.ArrayStrideInBytes,
				        .ScalarType = member.ScalarType,
				        .RowCount = member.RowCount,
				        .ColumnCount = member.ColumnCount});
			}
			result.ConstantBuffers.push_back(std::move(decoded));
		}
		const auto allBindings = map.GetResourceBindings();
		for (std::uint32_t index = 0; index < record.ResourceBindingCount; ++index)
		{
			const CookedShaderResourceBindingRecord& binding = allBindings[record.ResourceBindingOffset + index];
			std::uint32_t constantBufferIndex = kCookedShaderReflectionInvalidIndex;
			if (binding.ConstantBufferIndex >= record.ConstantBufferOffset
			    && binding.ConstantBufferIndex < record.ConstantBufferOffset + record.ConstantBufferCount)
			{
				constantBufferIndex = binding.ConstantBufferIndex - record.ConstantBufferOffset;
			}
			result.Bindings.push_back(
			    ShaderReflectionResourceBinding{
			        .Name = Resolve(map, binding.NameOffsetInBytes, binding.NameSizeInBytes),
			        .Kind = binding.Kind,
			        .Dimension = binding.Dimension,
			        .IsReadOnly = binding.IsReadOnly != 0,
			        .Set = binding.Set,
			        .Slot = binding.Slot,
			        .ArrayCount = binding.ArrayCount,
			        .SizeInBytes = binding.SizeInBytes,
			        .ConstantBufferIndex = constantBufferIndex});
		}
		const auto allInputs = map.GetInputElements();
		for (std::uint32_t index = 0; index < record.InputElementCount; ++index)
		{
			const CookedShaderInputElementRecord& input = allInputs[record.InputElementOffset + index];
			result.InputElements.push_back(
			    ShaderReflectionInputElement{
			        .Semantic = Resolve(map, input.SemanticOffsetInBytes, input.SemanticSizeInBytes),
			        .SemanticIndex = input.SemanticIndex,
			        .Location = input.Location,
			        .ScalarType = input.ScalarType,
			        .ComponentCount = input.ComponentCount});
		}
		const auto allPushConstants = map.GetPushConstantRanges();
		for (std::uint32_t index = 0; index < record.PushConstantRangeCount; ++index)
		{
			const CookedShaderPushConstantRangeRecord& range = allPushConstants[record.PushConstantRangeOffset + index];
			result.PushConstants.push_back(
			    ShaderReflectionPushConstantRange{
			        .OffsetInBytes = range.OffsetInBytes,
			        .SizeInBytes = range.SizeInBytes,
			        .VisibilityMask = range.VisibilityMask});
		}
		const auto allSpecializations = map.GetSpecializationConstants();
		for (std::uint32_t index = 0; index < record.SpecializationConstantCount; ++index)
		{
			const CookedShaderSpecializationConstantRecord& value = allSpecializations[record.SpecializationConstantOffset + index];
			result.SpecializationConstants.push_back(
			    ShaderReflectionSpecializationConstant{
			        .Name = Resolve(map, value.NameOffsetInBytes, value.NameSizeInBytes),
			        .ConstantId = value.ConstantId,
			        .DefaultValueBits = value.DefaultValueBits,
			        .ScalarType = value.ScalarType});
		}
		result.ThreadGroupSize = {record.ThreadGroupSize[0], record.ThreadGroupSize[1], record.ThreadGroupSize[2]};
		result.EntryFlags = record.EntryFlags;
		result.WaveSize = record.WaveSize;
		return result;
	}

	Entry FromExisting(const GlobalShaderMap& map, const CookedShaderLibrary& library, const GlobalShaderMapEntry& source)
	{
		Entry entry;
		entry.ShaderType = source.ShaderType;
		entry.Target = source.Target;
		entry.Stage = source.Stage;
		entry.Features = source.Features;
		entry.RayTracing = RayTracingShaderMetadata{
		    .PayloadSizeInBytes = source.RayPayloadSizeInBytes,
		    .AttributeSizeInBytes = source.RayAttributeSizeInBytes,
		    .MinimumRecursionDepth = source.MinimumRayRecursionDepth,
		    .LocalRecordSizeInBytes = source.LocalRecordSizeInBytes,
		    .LocalRecordSignature = source.LocalRecordSignature};
		entry.CodeHash = source.CodeHash;
		entry.ParameterSignature = source.ParameterSignature;
		entry.CompileInputHash = source.CompileInputHash;
		entry.BackendVersion = source.BackendVersion;
		entry.ShaderName = map.ResolveString(source.ShaderName);
		entry.EntryPoint = map.ResolveString(source.EntryPoint);
		entry.BackendName = map.ResolveString(source.BackendName);
		entry.CodegenTarget = map.ResolveString(source.CodegenTarget);
		for (const ShaderMapBindingRecord& binding : map.GetBindings(source))
		{
			entry.Bindings.push_back(
			    Binding{
			        .Name = std::string(map.ResolveString(binding.Name)),
			        .SemanticKind = binding.SemanticKind,
			        .ResourceDomain = binding.ResourceDomain,
			        .Access = binding.Access,
			        .VisibilityMask = binding.VisibilityMask,
			        .LogicalBindingIndex = binding.LogicalBindingIndex,
			        .ArrayCount = binding.ArrayCount,
			        .ValueSizeInBytes = binding.ValueSizeInBytes});
		}
		entry.Reflection = DecodeReflection(map, source);
		const CookedShaderCodeRecord* const code = library.Find(source.CodeHash);
		if (code == nullptr)
		{
			throw Diagnostics::Error("Existing global shader map references missing library code.");
		}
		const ShaderBytecode bytecode = library.GetBytecode(*code);
		entry.Code.assign(static_cast<const std::uint8_t*>(bytecode.Data), static_cast<const std::uint8_t*>(bytecode.Data) + bytecode.Size);
		return entry;
	}
}

namespace ShaderArtifactWriter
{
	ShaderMapStringRef ToStringRef(const Strings::StringTableEntry& entry) noexcept;
	void WriteMap(
	    const std::filesystem::path& path,
	    const GlobalShaderMapHeader& header,
	    const std::vector<GlobalShaderMapEntry>& entries,
	    const std::vector<ShaderMapBindingRecord>& bindings,
	    const ReflectionSerializer::Output& reflection,
	    const std::vector<std::uint8_t>& strings);
	void WriteLibrary(
	    const std::filesystem::path& path,
	    const CookedShaderLibraryHeader& header,
	    const std::vector<CookedShaderCodeRecord>& records,
	    const std::vector<std::uint8_t>& code);

	ShaderCookOutput Write(
	    std::vector<ShaderArtifactAssembly::Entry> entries,
	    const std::filesystem::path& mapPath,
	    const std::filesystem::path& libraryPath)
	{
		std::ranges::sort(
		    entries,
		    [](const ShaderArtifactAssembly::Entry& left, const ShaderArtifactAssembly::Entry& right)
		    { return left.ShaderType < right.ShaderType || (left.ShaderType == right.ShaderType && left.Target < right.Target); });
		const auto duplicate = std::adjacent_find(
		    entries.begin(),
		    entries.end(),
		    [](const ShaderArtifactAssembly::Entry& left, const ShaderArtifactAssembly::Entry& right)
		    { return left.ShaderType == right.ShaderType && left.Target == right.Target; });
		if (duplicate != entries.end())
		{
			throw Diagnostics::Error("Global shader map production contains a duplicate logical key.");
		}

		Strings::StringTableBuilder strings;
		std::vector<GlobalShaderMapEntry> mapEntries;
		std::vector<ShaderMapBindingRecord> bindings;
		std::vector<ShaderReflection> reflections;
		std::map<ShaderCodeHash, std::vector<std::uint8_t>> uniqueCode;
		ShaderCookOutput result{.mapPath = mapPath, .libraryPath = libraryPath};
		for (const ShaderArtifactAssembly::Entry& source : entries)
		{
			const auto [code, inserted] = uniqueCode.try_emplace(source.CodeHash, source.Code);
			if (!inserted && code->second != source.Code)
			{
				throw Diagnostics::Error("Two shader code records collided on one code hash.");
			}
			GlobalShaderMapEntry entry{};
			entry.ShaderType = source.ShaderType;
			entry.Target = source.Target;
			entry.Stage = source.Stage;
			entry.BinaryFormat = GetShaderBinaryFormat(source.Target);
			entry.Features = source.Features;
			entry.RayPayloadSizeInBytes = source.RayTracing.PayloadSizeInBytes;
			entry.RayAttributeSizeInBytes = source.RayTracing.AttributeSizeInBytes;
			entry.MinimumRayRecursionDepth = source.RayTracing.MinimumRecursionDepth;
			entry.LocalRecordSizeInBytes = source.RayTracing.LocalRecordSizeInBytes;
			entry.LocalRecordSignature = source.RayTracing.LocalRecordSignature;
			entry.CodeHash = source.CodeHash;
			entry.ParameterSignature = source.ParameterSignature;
			entry.CompileInputHash = source.CompileInputHash;
			entry.BackendVersion = source.BackendVersion;
			entry.ShaderName = ToStringRef(strings.Add(source.ShaderName));
			entry.EntryPoint = ToStringRef(strings.Add(source.EntryPoint));
			entry.BackendName = ToStringRef(strings.Add(source.BackendName));
			entry.CodegenTarget = ToStringRef(strings.Add(source.CodegenTarget));
			entry.BindingRecordOffset = static_cast<std::uint32_t>(bindings.size());
			entry.BindingRecordCount = static_cast<std::uint32_t>(source.Bindings.size());
			entry.ReflectionRecordIndex = static_cast<std::uint32_t>(reflections.size());
			for (const ShaderArtifactAssembly::Binding& sourceBinding : source.Bindings)
			{
				bindings.push_back(
				    ShaderMapBindingRecord{
				        .Name = ToStringRef(strings.Add(sourceBinding.Name)),
				        .SemanticKind = sourceBinding.SemanticKind,
				        .ResourceDomain = sourceBinding.ResourceDomain,
				        .Access = sourceBinding.Access,
				        .VisibilityMask = sourceBinding.VisibilityMask,
				        .LogicalBindingIndex = sourceBinding.LogicalBindingIndex,
				        .ArrayCount = sourceBinding.ArrayCount,
				        .ValueSizeInBytes = sourceBinding.ValueSizeInBytes});
			}
			mapEntries.push_back(entry);
			reflections.push_back(source.Reflection);
			result.entries.push_back(
			    ShaderCookedEntry{
			        .shaderType = source.ShaderType,
			        .shaderName = source.ShaderName,
			        .target = source.Target,
			        .codeHash = source.CodeHash,
			        .codeSizeInBytes = source.Code.size()});
		}

		ReflectionSerializer::Output reflectionOutput;
		ReflectionSerializer::Build(reflections, strings, reflectionOutput);
		std::vector<CookedShaderCodeRecord> codeRecords;
		std::vector<std::uint8_t> codeBlob;
		for (const auto& [hash, bytes] : uniqueCode)
		{
			const std::uint32_t offset = static_cast<std::uint32_t>(codeBlob.size());
			codeBlob.insert(codeBlob.end(), bytes.begin(), bytes.end());
			codeRecords.push_back(
			    CookedShaderCodeRecord{.CodeHash = hash, .Code = ShaderCodeBlobRef{offset, static_cast<std::uint32_t>(bytes.size())}});
		}
		result.uniqueCodeCount = codeRecords.size();

		std::uint64_t publicationHash = Hash::kFnv64OffsetBasis;
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, mapEntries);
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, bindings);
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, reflectionOutput.reflectionRecords);
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, reflectionOutput.resourceBindings);
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, reflectionOutput.constantBuffers);
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, reflectionOutput.constantBufferMembers);
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, reflectionOutput.inputElements);
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, reflectionOutput.pushConstantRanges);
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, reflectionOutput.specializationConstants);
		publicationHash = Hash::ContinueFnv1a64(publicationHash, strings.GetBytes().data(), strings.GetBytes().size());
		publicationHash = Hash::ContinueFnv1a64Vector(publicationHash, codeRecords);
		publicationHash = Hash::ContinueFnv1a64(publicationHash, codeBlob.data(), codeBlob.size());
		publicationHash = Hash::FinalizeFnv1a64(publicationHash);

		GlobalShaderMapHeader mapHeader{};
		mapHeader.EntryCount = static_cast<std::uint32_t>(mapEntries.size());
		mapHeader.PublicationHash = publicationHash;
		mapHeader.BindingRecordCount = static_cast<std::uint32_t>(bindings.size());
		mapHeader.ReflectionRecordCount = static_cast<std::uint32_t>(reflectionOutput.reflectionRecords.size());
		mapHeader.ResourceBindingRecordCount = static_cast<std::uint32_t>(reflectionOutput.resourceBindings.size());
		mapHeader.ConstantBufferRecordCount = static_cast<std::uint32_t>(reflectionOutput.constantBuffers.size());
		mapHeader.ConstantBufferMemberRecordCount = static_cast<std::uint32_t>(reflectionOutput.constantBufferMembers.size());
		mapHeader.InputElementRecordCount = static_cast<std::uint32_t>(reflectionOutput.inputElements.size());
		mapHeader.PushConstantRangeRecordCount = static_cast<std::uint32_t>(reflectionOutput.pushConstantRanges.size());
		mapHeader.SpecializationConstantRecordCount = static_cast<std::uint32_t>(reflectionOutput.specializationConstants.size());
		mapHeader.StringTableSizeInBytes = strings.SizeInBytes();
		CookedShaderLibraryHeader libraryHeader{};
		libraryHeader.RecordCount = static_cast<std::uint32_t>(codeRecords.size());
		libraryHeader.PublicationHash = publicationHash;
		libraryHeader.CodeBlobSizeInBytes = static_cast<std::uint32_t>(codeBlob.size());

		WriteMap(mapPath, mapHeader, mapEntries, bindings, reflectionOutput, strings.GetBytes());
		WriteLibrary(libraryPath, libraryHeader, codeRecords, codeBlob);
		return result;
	}

	ShaderMapStringRef ToStringRef(const Strings::StringTableEntry& entry) noexcept
	{
		return ShaderMapStringRef{entry.OffsetInBytes, entry.SizeInBytes};
	}

	void WriteMap(
	    const std::filesystem::path& path,
	    const GlobalShaderMapHeader& header,
	    const std::vector<GlobalShaderMapEntry>& entries,
	    const std::vector<ShaderMapBindingRecord>& bindings,
	    const ReflectionSerializer::Output& reflection,
	    const std::vector<std::uint8_t>& strings)
	{
		std::ofstream output;
		std::string error;
		if (!Files::TryOpenBinaryOutput(path, output, error) || !Files::BinaryStreamWriter::WriteValue(output, header, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, entries, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, bindings, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, reflection.reflectionRecords, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, reflection.resourceBindings, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, reflection.constantBuffers, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, reflection.constantBufferMembers, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, reflection.inputElements, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, reflection.pushConstantRanges, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, reflection.specializationConstants, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, strings, error) || !Files::TryCloseOutput(output, path, error))
		{
			throw Diagnostics::Error(error);
		}
	}

	void WriteLibrary(
	    const std::filesystem::path& path,
	    const CookedShaderLibraryHeader& header,
	    const std::vector<CookedShaderCodeRecord>& records,
	    const std::vector<std::uint8_t>& code)
	{
		std::ofstream output;
		std::string error;
		if (!Files::TryOpenBinaryOutput(path, output, error) || !Files::BinaryStreamWriter::WriteValue(output, header, error)
		    || !Files::BinaryStreamWriter::WriteArray(output, records, error) || !Files::BinaryStreamWriter::WriteArray(output, code, error)
		    || !Files::TryCloseOutput(output, path, error))
		{
			throw Diagnostics::Error(error);
		}
	}
}

ShaderCookOutput ShaderArtifactPublication::Publish(
    const ShaderCookPipelinePlan& plan,
    const std::filesystem::path& outputDirectory,
    bool replaceCompleteCatalog)
{
	const std::filesystem::path& mapPath = Filesystem::GetGlobalShaderMapPath();
	const std::filesystem::path& libraryPath = Filesystem::GetCookedShaderLibraryPath();
	const std::filesystem::path stagedMapPath = Files::BuildTemporaryPath(mapPath, ".cook-generation");
	const std::filesystem::path stagedLibraryPath = Files::BuildTemporaryPath(libraryPath, ".cook-generation");
	const std::filesystem::path dependencyPath = ShaderDependencyManifest::GetPath(outputDirectory);
	const std::filesystem::path stagedDependencyPath = Files::BuildTemporaryPath(dependencyPath, ".cook-generation");
	const std::filesystem::path signalPath = Paths::ShaderRecookSignal(outputDirectory);
	const std::filesystem::path stagedSignalPath = Files::BuildTemporaryPath(signalPath, ".cook-generation");
	const auto cleanupStagedFiles = [&]() noexcept
	{
		Files::CleanupTemporaryFile(stagedMapPath);
		Files::CleanupTemporaryFile(stagedLibraryPath);
		Files::CleanupTemporaryFile(stagedDependencyPath);
		Files::CleanupTemporaryFile(stagedSignalPath);
	};
	cleanupStagedFiles();

	try
	{
		std::map<std::pair<ShaderTypeId, ShaderTarget>, ShaderArtifactAssembly::Entry> entries;
		if (!replaceCompleteCatalog)
		{
			const CookedShaderLibrary existingLibrary = CookedShaderLibrary::Open(libraryPath);
			const GlobalShaderMap existingMap = GlobalShaderMap::Open(mapPath, existingLibrary);
			for (const GlobalShaderMapEntry& entry : existingMap.GetEntries())
			{
				if (!std::ranges::binary_search(plan.registeredShaderTypes, entry.ShaderType))
				{
					continue;
				}
				entries.emplace(
				    std::pair{entry.ShaderType, entry.Target},
				    ShaderArtifactAssembly::FromExisting(existingMap, existingLibrary, entry));
			}
		}
		for (std::size_t shaderIndex = 0; shaderIndex < plan.shaders.size(); ++shaderIndex)
		{
			for (const ShaderCookProduct& product : plan.shaderOutputs[shaderIndex])
			{
				ShaderArtifactAssembly::Entry entry = ShaderArtifactAssembly::FromProduct(plan.shaders[shaderIndex], product);
				entries.insert_or_assign(std::pair{entry.ShaderType, entry.Target}, std::move(entry));
			}
		}
		std::vector<ShaderArtifactAssembly::Entry> orderedEntries;
		for (auto& [key, entry] : entries)
		{
			(void) key;
			orderedEntries.push_back(std::move(entry));
		}

		ShaderCookOutput result = ShaderArtifactWriter::Write(std::move(orderedEntries), stagedMapPath, stagedLibraryPath);
		result.mapPath = mapPath;
		result.libraryPath = libraryPath;
		const CookedShaderLibrary stagedLibrary = CookedShaderLibrary::Open(stagedLibraryPath);
		(void) GlobalShaderMap::Open(stagedMapPath, stagedLibrary);
		ShaderDependencyManifest::Write(plan.dependencyManifest, stagedDependencyPath);
		ShaderRecookSignal::Write(stagedMapPath, mapPath, stagedLibraryPath, libraryPath, stagedSignalPath);
		const std::vector<Files::FilePublication> files = {
		    {stagedMapPath, mapPath},
		    {stagedLibraryPath, libraryPath},
		    {stagedDependencyPath, dependencyPath},
		    {stagedSignalPath, signalPath}};
		std::string error;
		if (!Files::TryPublishFileSet(files, error))
		{
			throw Diagnostics::Error(error);
		}
		return result;
	}
	catch (...)
	{
		cleanupStagedFiles();
		throw;
	}
}
