#pragma once

#include <stdexcept>

namespace Diagnostics
{
	class Error final : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};
}
