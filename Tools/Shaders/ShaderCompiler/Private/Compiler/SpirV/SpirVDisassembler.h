#pragma once

#include <cstdint>
#include <span>
#include <string>

class SpirVDisassembler final
{
public:
	SpirVDisassembler() = delete;

	static std::string Disassemble(std::span<const std::uint8_t> bytecode);
};
