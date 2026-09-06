#pragma once

#include "Core/Public/CoreAPI.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

namespace Files
{
	class BinaryStreamWriter final
	{
	public:
		static SPARKLE_CORE_API bool WriteBytes(
		    std::ofstream& output,
		    const void* bytes,
		    std::size_t byteCount,
		    std::string& outErrorMessage);

		template <typename T> static bool WriteValue(std::ofstream& output, const T& value, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinaryStreamWriter requires trivially-copyable values");
			output.write(reinterpret_cast<const char*>(&value), sizeof(T));
			if (output.good())
			{
				return true;
			}

			outErrorMessage = "Failed to write binary value to output stream";
			return false;
		}

		template <typename T> static bool WriteArray(std::ofstream& output, const std::vector<T>& values, std::string& outErrorMessage)
		{
			static_assert(std::is_trivially_copyable_v<T>, "BinaryStreamWriter requires trivially-copyable values");
			if (values.empty())
			{
				return true;
			}

			output.write(reinterpret_cast<const char*>(values.data()), static_cast<std::streamsize>(sizeof(T) * values.size()));
			if (output.good())
			{
				return true;
			}

			outErrorMessage = "Failed to write binary array to output stream";
			return false;
		}
	};
}
