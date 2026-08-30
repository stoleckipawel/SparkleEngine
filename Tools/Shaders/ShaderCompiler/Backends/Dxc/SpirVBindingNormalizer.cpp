#include "PCH.h"

#include "SpirVBindingNormalizer.h"

#include "Core/Public/Diagnostics/Error.h"

#include <spirv_reflect.h>

#include <algorithm>
#include <cstring>

void SpirVBindingNormalizer::Normalize(std::vector<std::uint8_t>& bytecode, std::span<const ShaderDescriptorBindingRemap> remaps)
{
	if (bytecode.empty() || remaps.empty())
	{
		return;
	}

	SpvReflectShaderModule module{};
	if (spvReflectCreateShaderModule(bytecode.size(), bytecode.data(), &module) != SPV_REFLECT_RESULT_SUCCESS)
	{
		throw Diagnostics::Error("spvReflectCreateShaderModule failed");
	}

	std::uint32_t bindingCount = 0;
	SpvReflectResult result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, nullptr);
	std::vector<SpvReflectDescriptorBinding*> bindings(bindingCount, nullptr);
	if (result == SPV_REFLECT_RESULT_SUCCESS && bindingCount > 0)
	{
		result = spvReflectEnumerateDescriptorBindings(&module, &bindingCount, bindings.data());
	}
	if (result != SPV_REFLECT_RESULT_SUCCESS)
	{
		spvReflectDestroyShaderModule(&module);
		throw Diagnostics::Error("spvReflectEnumerateDescriptorBindings failed");
	}

	for (SpvReflectDescriptorBinding* binding : bindings)
	{
		if (binding == nullptr || binding->name == nullptr)
		{
			continue;
		}
		const auto remap = std::ranges::find_if(
		    remaps,
		    [binding](const ShaderDescriptorBindingRemap& candidate) { return candidate.Name == binding->name; });
		if (remap == remaps.end())
		{
			continue;
		}
		result = spvReflectChangeDescriptorBindingNumbers(&module, binding, remap->Binding, remap->Set);
		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			spvReflectDestroyShaderModule(&module);
			throw Diagnostics::Error("spvReflectChangeDescriptorBindingNumbers failed for '" + remap->Name + "'");
		}
	}

	const std::uint32_t normalizedSize = spvReflectGetCodeSize(&module);
	const std::uint32_t* const normalizedCode = spvReflectGetCode(&module);
	if (normalizedCode == nullptr || normalizedSize == 0)
	{
		spvReflectDestroyShaderModule(&module);
		throw Diagnostics::Error("spvReflectGetCode returned empty bytecode");
	}
	bytecode.resize(normalizedSize);
	std::memcpy(bytecode.data(), normalizedCode, normalizedSize);
	spvReflectDestroyShaderModule(&module);
}
