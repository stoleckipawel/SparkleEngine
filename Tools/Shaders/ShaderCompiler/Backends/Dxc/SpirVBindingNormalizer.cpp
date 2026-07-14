#include "PCH.h"

#include "SpirVBindingNormalizer.h"

#include <spirv_reflect.h>

#include <algorithm>
#include <cstring>

bool SpirVBindingNormalizer::Normalize(
    std::vector<std::uint8_t>& bytecode,
    std::span<const ShaderDescriptorBindingRemap> remaps,
    std::string& outErrorMessage)
{
	if (bytecode.empty() || remaps.empty())
	{
		outErrorMessage.clear();
		return true;
	}

	SpvReflectShaderModule module{};
	if (spvReflectCreateShaderModule(bytecode.size(), bytecode.data(), &module) != SPV_REFLECT_RESULT_SUCCESS)
	{
		outErrorMessage = "spvReflectCreateShaderModule failed";
		return false;
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
		outErrorMessage = "spvReflectEnumerateDescriptorBindings failed";
		return false;
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
			outErrorMessage = "spvReflectChangeDescriptorBindingNumbers failed for '" + remap->Name + "'";
			return false;
		}
	}

	const std::uint32_t normalizedSize = spvReflectGetCodeSize(&module);
	const std::uint32_t* const normalizedCode = spvReflectGetCode(&module);
	if (normalizedCode == nullptr || normalizedSize == 0)
	{
		spvReflectDestroyShaderModule(&module);
		outErrorMessage = "spvReflectGetCode returned empty bytecode";
		return false;
	}
	bytecode.resize(normalizedSize);
	std::memcpy(bytecode.data(), normalizedCode, normalizedSize);
	spvReflectDestroyShaderModule(&module);
	outErrorMessage.clear();
	return true;
}
