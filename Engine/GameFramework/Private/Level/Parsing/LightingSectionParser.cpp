#include "PCH.h"

#include "Level/Parsing/LightingSectionParser.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Level/Parsing/Lighting/LightFieldKeyParser.h"
#include "Level/Parsing/Lighting/LightFieldParser.h"
#include "Level/Parsing/Lighting/LightingSectionWriter.h"

#include <format>

namespace LevelParsing
{
	class LightingSectionValidation final
	{
	  public:
		static void ValidateRectBasis(
		    const DirectX::XMFLOAT3& direction,
		    const DirectX::XMFLOAT3& tangent,
		    std::size_t lightIndex)
		{
			const DirectX::XMVECTOR cross =
			    DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&direction), DirectX::XMLoadFloat3(&tangent));
			if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(cross)) <= 1.0e-8f)
				throw Diagnostics::Error(
				    std::format("Rect light {} has parallel direction and tangent vectors.", lightIndex));
		}
	};

	void ParseLightingSectionField(const ParsedLevelLine& parsedLine, LevelDesc& levelDesc)
	{
		const ParsedLightFieldKey key = ParseLightFieldKey(parsedLine.key);
		ParseLightField(key, parsedLine, levelDesc.lights);
	}

	void ValidateLightingSection(const LevelDesc& levelDesc)
	{
		for (std::size_t lightIndex = 0; lightIndex < levelDesc.lights.size(); ++lightIndex)
		{
			const SceneLightDesc& light = levelDesc.lights[lightIndex];
			if (const SpotLightDesc* spot = light.GetSpot(); spot != nullptr && spot->innerAngleRadians > spot->outerAngleRadians)
				throw Diagnostics::Error(
				    std::format("Spot light {} has an inner cone wider than its outer cone.", lightIndex));
			if (const RectLightDesc* rect = light.GetRect(); rect != nullptr)
				LightingSectionValidation::ValidateRectBasis(rect->direction, rect->tangent, lightIndex);
		}
	}

	void WriteLightingSection(std::ofstream& output, const LevelDesc& levelDesc)
	{
		WriteLightingSectionValues(output, levelDesc);
	}
}
