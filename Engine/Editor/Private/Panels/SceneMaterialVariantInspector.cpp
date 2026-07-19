#include "PCH.h"

#include "Panels/SceneMaterialVariantInspector.h"

#include "World/GameWorld.h"
#include "Util/UiUtil.h"

#include <imgui.h>

namespace SceneMaterialVariantInspector
{
	void Build(GameWorld& gameWorld) noexcept
	{
		const std::size_t variantCount = gameWorld.GetMaterialVariantCount();
		if (variantCount == 0)
		{
			UiUtil::DrawDetailsEmptyState();
			return;
		}

		if (!UiUtil::BeginDetailsCategory("Material Variants"))
		{
			return;
		}

		const MaterialVariantIndex activeVariantIndex = gameWorld.GetActiveMaterialVariant();
		const char* preview = "Select Variant";
		std::string activeVariantName;
		if (activeVariantIndex < variantCount)
		{
			activeVariantName = gameWorld.GetMaterialVariantName(activeVariantIndex);
			preview = activeVariantName.c_str();
		}

		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("##SceneMaterialVariant", preview))
		{
			for (std::size_t variantIndex = 0; variantIndex < variantCount; ++variantIndex)
			{
				const std::string variantName(gameWorld.GetMaterialVariantName(variantIndex));
				const bool selected = variantIndex == activeVariantIndex;
				if (ImGui::Selectable(variantName.c_str(), selected))
				{
					gameWorld.ApplyMaterialVariant(static_cast<MaterialVariantIndex>(variantIndex));
				}

				if (selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		UiUtil::EndDetailsCategory();
	}
}
