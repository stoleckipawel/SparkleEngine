#pragma once

#include <type_traits>

#define RHI_CBV_CHECK(Type)                                                                  \
	static_assert(std::is_standard_layout_v<Type>, #Type " must be standard-layout");        \
	static_assert(std::is_trivially_copyable_v<Type>, #Type " must be trivially-copyable");  \
	static_assert(alignof(Type) >= 256, #Type " must be 256-byte aligned");                  \
	static_assert(sizeof(Type) % 256 == 0, #Type " must occupy whole 256-byte CBV slot(s)"); \
	static_assert(sizeof(Type) <= 64 * 1024, #Type " must be <= 64KB")
