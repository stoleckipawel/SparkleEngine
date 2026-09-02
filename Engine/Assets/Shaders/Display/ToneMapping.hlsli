#ifndef SPARKLE_DISPLAY_TONE_MAPPING_HLSLI
#define SPARKLE_DISPLAY_TONE_MAPPING_HLSLI

#include "/Engine/Common/Color.hlsli"

namespace ToneMapping
{
	static const uint ToneMapperReinhard = 0u;
	static const uint ToneMapperAcesApprox = 1u;
	static const uint ToneMapperAcesFilmic = 2u;

	float3 Reinhard(float3 color)
	{
		const float3 positiveColor = CommonColor::ClampPositive(color);
		return positiveColor / (positiveColor + 1.0f.xxx);
	}

	float3 AcesApprox(float3 color)
	{
		const float3 x = CommonColor::ClampPositive(color);
		const float a = 2.51f;
		const float b = 0.03f;
		const float c = 2.43f;
		const float d = 0.59f;
		const float e = 0.14f;
		return saturate((x * (a * x + b)) / max(x * (c * x + d) + e, 1.0e-6f.xxx));
	}

	float3 RrtAndOdtFit(float3 color)
	{
		const float3 a = color * (color + 0.0245786f.xxx) - 0.000090537f.xxx;
		const float3 b = color * (0.983729f * color + 0.4329510f.xxx) + 0.238081f.xxx;
		return a / max(b, 1.0e-6f.xxx);
	}

	float3 AcesFilmic(float3 color)
	{
		const float3x3 acesInputMatrix = float3x3(0.59719f, 0.35458f, 0.04823f, 0.07600f, 0.90834f, 0.01566f, 0.02840f, 0.13383f, 0.83777f);
		const float3x3 acesOutputMatrix =
		    float3x3(1.60475f, -0.53108f, -0.07367f, -0.10208f, 1.10813f, -0.00605f, -0.00327f, -0.07276f, 1.07602f);

		float3 fittedColor = mul(acesInputMatrix, CommonColor::ClampPositive(color));
		fittedColor = RrtAndOdtFit(fittedColor);
		fittedColor = mul(acesOutputMatrix, fittedColor);
		return saturate(fittedColor);
	}

	float3 ApplyToneMapper(float3 color, uint toneMapper)
	{
		if (toneMapper == ToneMapperReinhard)
		{
			return Reinhard(color);
		}
		if (toneMapper == ToneMapperAcesFilmic)
		{
			return AcesFilmic(color);
		}
		return AcesApprox(color);
	}
}

#endif
