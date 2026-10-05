#include "PCH.h"
#include "Compiler/SpirV/SpirVDisassembler.h"

#include "Core/Public/Diagnostics/Error.h"

#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/libspirv.h>

#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <vector>

std::string SpirVDisassembler::Disassemble(std::span<const std::uint8_t> bytecode)
{
	if (bytecode.empty() || bytecode.size() % sizeof(std::uint32_t) != 0u)
	{
		throw Diagnostics::Error("SPIR-V disassembly requires nonempty word-aligned bytecode.");
	}

	// The native decoder requires aligned words, while compiler results own bytes.
	std::vector<std::uint32_t> words(bytecode.size() / sizeof(std::uint32_t));
	std::memcpy(words.data(), bytecode.data(), bytecode.size());
	spvtools::SpirvTools tools(SPV_ENV_UNIVERSAL_1_6);
	std::string diagnostic;
	tools.SetMessageConsumer(
	    [&diagnostic](spv_message_level_t, const char*, const spv_position_t&, const char* message)
	    {
		    if (message != nullptr)
		    {
			    diagnostic = message;
		    }
	    });

	std::string disassembly;
	const auto options =
	    static_cast<std::uint32_t>(SPV_BINARY_TO_TEXT_OPTION_INDENT) | static_cast<std::uint32_t>(SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
	if (!tools.Disassemble(words, &disassembly, options) || disassembly.empty())
	{
		throw Diagnostics::Error("SPIR-V disassembly failed: " + diagnostic);
	}
	return disassembly;
}
