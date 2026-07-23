#include "Capture/RhiBmpWriter.h"

#include <algorithm>
#include <fstream>
#include <vector>

class RhiBmpWriterImplementation final
{
  public:
#pragma pack(push, 1)
	struct BmpFileHeader final
	{
		std::uint16_t Type = 0x4D42;
		std::uint32_t Size = 0;
		std::uint16_t Reserved1 = 0;
		std::uint16_t Reserved2 = 0;
		std::uint32_t OffBits = 54;
	};

	struct BmpInfoHeader final
	{
		std::uint32_t Size = sizeof(BmpInfoHeader);
		std::int32_t Width = 0;
		std::int32_t Height = 0;
		std::uint16_t Planes = 1;
		std::uint16_t BitCount = 32;
		std::uint32_t Compression = 0;
		std::uint32_t SizeImage = 0;
		std::int32_t XPelsPerMeter = 2835;
		std::int32_t YPelsPerMeter = 2835;
		std::uint32_t ClrUsed = 0;
		std::uint32_t ClrImportant = 0;
	};
#pragma pack(pop)

	static std::byte ToByte(float value) noexcept
	{
		const float clamped = std::clamp(value, 0.0f, 1.0f);
		return static_cast<std::byte>(static_cast<std::uint32_t>(clamped * 255.0f + 0.5f));
	}

	static bool ConvertPixel(
	    const std::byte* sourcePixel,
	    RhiBmpSourceFormat sourceFormat,
	    std::byte* outputPixel) noexcept
	{
		if (sourcePixel == nullptr || outputPixel == nullptr)
		{
			return false;
		}

		switch (sourceFormat)
		{
		case RhiBmpSourceFormat::Rgba32Float:
		{
			const float* rgba = reinterpret_cast<const float*>(sourcePixel);
			outputPixel[0] = ToByte(rgba[2]);
			outputPixel[1] = ToByte(rgba[1]);
			outputPixel[2] = ToByte(rgba[0]);
			outputPixel[3] = ToByte(rgba[3]);
			return true;
		}
		case RhiBmpSourceFormat::Rgba8Unorm:
			outputPixel[0] = sourcePixel[2];
			outputPixel[1] = sourcePixel[1];
			outputPixel[2] = sourcePixel[0];
			outputPixel[3] = sourcePixel[3];
			return true;
		case RhiBmpSourceFormat::Bgra8Unorm:
			outputPixel[0] = sourcePixel[0];
			outputPixel[1] = sourcePixel[1];
			outputPixel[2] = sourcePixel[2];
			outputPixel[3] = sourcePixel[3];
			return true;
		}

		return false;
	}

	static std::uint32_t GetSourcePixelStride(RhiBmpSourceFormat sourceFormat) noexcept
	{
		return sourceFormat == RhiBmpSourceFormat::Rgba32Float ? 16u : 4u;
	}
};

bool WriteRhiBmp(
    const std::filesystem::path& outputPath,
    const std::byte* sourcePixels,
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t sourceRowPitch,
    RhiBmpSourceFormat sourceFormat) noexcept
{
	if (sourcePixels == nullptr || width == 0 || height == 0 || sourceRowPitch == 0)
	{
		return false;
	}

	std::error_code error;
	if (const std::filesystem::path parentPath = outputPath.parent_path(); !parentPath.empty())
	{
		std::filesystem::create_directories(parentPath, error);
		if (error)
		{
			return false;
		}
	}

	const std::uint32_t sourcePixelStride = RhiBmpWriterImplementation::GetSourcePixelStride(sourceFormat);
	const std::uint32_t outputRowPitch = width * 4u;
	std::vector<std::byte> outputPixels(static_cast<std::size_t>(outputRowPitch) * height);
	for (std::uint32_t y = 0; y < height; ++y)
	{
		const std::byte* sourceRow = sourcePixels + static_cast<std::size_t>(sourceRowPitch) * y;
		std::byte* outputRow = outputPixels.data() + static_cast<std::size_t>(outputRowPitch) * y;
		for (std::uint32_t x = 0; x < width; ++x)
		{
			const std::byte* sourcePixel = sourceRow + static_cast<std::size_t>(x) * sourcePixelStride;
			std::byte* outputPixel = outputRow + static_cast<std::size_t>(x) * 4u;
			if (!RhiBmpWriterImplementation::ConvertPixel(sourcePixel, sourceFormat, outputPixel))
			{
				return false;
			}
		}
	}

	RhiBmpWriterImplementation::BmpFileHeader fileHeader{};
	RhiBmpWriterImplementation::BmpInfoHeader infoHeader{};
	infoHeader.Width = static_cast<std::int32_t>(width);
	infoHeader.Height = -static_cast<std::int32_t>(height);
	infoHeader.SizeImage = static_cast<std::uint32_t>(outputPixels.size());
	fileHeader.Size = fileHeader.OffBits + infoHeader.SizeImage;

	std::ofstream output(outputPath, std::ios::binary);
	if (!output)
	{
		return false;
	}

	output.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
	output.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
	output.write(reinterpret_cast<const char*>(outputPixels.data()), static_cast<std::streamsize>(outputPixels.size()));
	return output.good();
}
