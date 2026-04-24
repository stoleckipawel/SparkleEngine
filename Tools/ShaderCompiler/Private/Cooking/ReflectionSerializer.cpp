#include "PCH.h"

#include "Cooking/ReflectionSerializer.h"

void ReflectionSerializer::Build(
    std::span<const ShaderReflection> reflections,
    Engine::Strings::StringTableBuilder& stringTable,
    Output& outOutput)
{
	outOutput.reflectionRecords.clear();
	outOutput.resourceBindings.clear();
	outOutput.constantBuffers.clear();
	outOutput.constantBufferMembers.clear();
	outOutput.inputElements.clear();
	outOutput.pushConstantRanges.clear();
	outOutput.specializationConstants.clear();

	outOutput.reflectionRecords.reserve(reflections.size());

	for (const ShaderReflection& reflection : reflections)
	{
		// Emit constant buffers before bindings so CB indices are already stable.
		const std::uint32_t cbBaseIndex = static_cast<std::uint32_t>(outOutput.constantBuffers.size());
		for (const ShaderReflectionConstantBuffer& cb : reflection.ConstantBuffers)
		{
			const auto nameEntry = stringTable.Add(cb.Name);
			const std::uint32_t memberOffset = static_cast<std::uint32_t>(outOutput.constantBufferMembers.size());

			for (const ShaderReflectionConstantBufferMember& member : cb.Members)
			{
				const auto memberNameEntry = stringTable.Add(member.Name);
				outOutput.constantBufferMembers.push_back(
				    CookedShaderConstantBufferMemberRecord{
				        .NameOffsetInBytes = memberNameEntry.OffsetInBytes,
				        .NameSizeInBytes = memberNameEntry.SizeInBytes,
				        .OffsetInBytes = member.OffsetInBytes,
				        .SizeInBytes = member.SizeInBytes,
				        .ArrayCount = member.ArrayCount,
				        .ArrayStrideInBytes = member.ArrayStrideInBytes,
				        .ScalarType = member.ScalarType,
				        .RowCount = member.RowCount,
				        .ColumnCount = member.ColumnCount});
			}

			outOutput.constantBuffers.push_back(
			    CookedShaderConstantBufferRecord{
			        .NameOffsetInBytes = nameEntry.OffsetInBytes,
			        .NameSizeInBytes = nameEntry.SizeInBytes,
			        .MemberOffset = memberOffset,
			        .MemberCount = static_cast<std::uint32_t>(cb.Members.size()),
			        .SizeInBytes = cb.SizeInBytes});
		}

		const std::uint32_t bindingOffset = static_cast<std::uint32_t>(outOutput.resourceBindings.size());
		for (const ShaderReflectionResourceBinding& binding : reflection.Bindings)
		{
			const auto nameEntry = stringTable.Add(binding.Name);
			std::uint32_t cbIndex = kCookedShaderReflectionInvalidIndex;
			if (binding.ConstantBufferIndex != kCookedShaderReflectionInvalidIndex)
			{
				cbIndex = cbBaseIndex + binding.ConstantBufferIndex;
			}
			outOutput.resourceBindings.push_back(
			    CookedShaderResourceBindingRecord{
			        .NameOffsetInBytes = nameEntry.OffsetInBytes,
			        .NameSizeInBytes = nameEntry.SizeInBytes,
			        .Kind = binding.Kind,
			        .Dimension = binding.Dimension,
			        .IsReadOnly = binding.IsReadOnly ? std::uint8_t{1} : std::uint8_t{0},
			        .Set = binding.Set,
			        .Slot = binding.Slot,
			        .ArrayCount = binding.ArrayCount,
			        .SizeInBytes = binding.SizeInBytes,
			        .ConstantBufferIndex = cbIndex});
		}

		const std::uint32_t inputOffset = static_cast<std::uint32_t>(outOutput.inputElements.size());
		for (const ShaderReflectionInputElement& element : reflection.InputElements)
		{
			const auto semanticEntry = stringTable.Add(element.Semantic);
			outOutput.inputElements.push_back(
			    CookedShaderInputElementRecord{
			        .SemanticOffsetInBytes = semanticEntry.OffsetInBytes,
			        .SemanticSizeInBytes = semanticEntry.SizeInBytes,
			        .SemanticIndex = element.SemanticIndex,
			        .Location = element.Location,
			        .ScalarType = element.ScalarType,
			        .ComponentCount = element.ComponentCount});
		}

		const std::uint32_t pushOffset = static_cast<std::uint32_t>(outOutput.pushConstantRanges.size());
		for (const ShaderReflectionPushConstantRange& range : reflection.PushConstants)
		{
			outOutput.pushConstantRanges.push_back(
			    CookedShaderPushConstantRangeRecord{
			        .OffsetInBytes = range.OffsetInBytes,
			        .SizeInBytes = range.SizeInBytes,
			        .VisibilityMask = range.VisibilityMask});
		}

		const std::uint32_t specOffset = static_cast<std::uint32_t>(outOutput.specializationConstants.size());
		for (const ShaderReflectionSpecializationConstant& spec : reflection.SpecializationConstants)
		{
			const auto nameEntry = stringTable.Add(spec.Name);
			outOutput.specializationConstants.push_back(
			    CookedShaderSpecializationConstantRecord{
			        .NameOffsetInBytes = nameEntry.OffsetInBytes,
			        .NameSizeInBytes = nameEntry.SizeInBytes,
			        .ConstantId = spec.ConstantId,
			        .DefaultValueBits = spec.DefaultValueBits,
			        .ScalarType = spec.ScalarType});
		}

		CookedShaderReflectionRecord record{};
		record.ResourceBindingOffset = bindingOffset;
		record.ResourceBindingCount = static_cast<std::uint32_t>(reflection.Bindings.size());
		record.ConstantBufferOffset = cbBaseIndex;
		record.ConstantBufferCount = static_cast<std::uint32_t>(reflection.ConstantBuffers.size());
		record.InputElementOffset = inputOffset;
		record.InputElementCount = static_cast<std::uint32_t>(reflection.InputElements.size());
		record.PushConstantRangeOffset = pushOffset;
		record.PushConstantRangeCount = static_cast<std::uint32_t>(reflection.PushConstants.size());
		record.SpecializationConstantOffset = specOffset;
		record.SpecializationConstantCount = static_cast<std::uint32_t>(reflection.SpecializationConstants.size());
		record.ThreadGroupSize[0] = reflection.ThreadGroupSize[0];
		record.ThreadGroupSize[1] = reflection.ThreadGroupSize[1];
		record.ThreadGroupSize[2] = reflection.ThreadGroupSize[2];
		record.EntryFlags = reflection.EntryFlags;
		record.WaveSize = reflection.WaveSize;
		outOutput.reflectionRecords.push_back(record);
	}
}
