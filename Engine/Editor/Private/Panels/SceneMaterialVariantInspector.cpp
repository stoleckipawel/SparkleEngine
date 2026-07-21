#include "PCH.h"

#include "Panels/SceneMaterialVariantInspector.h"

#include "Scene/Transactions/EditorTransactionManager.h"
#include "World/WorldMaterialVariantView.h"
#include "Util/UiUtil.h"

#include <imgui.h>

namespace SceneMaterialVariantInspector
{
	void Build(const WorldMaterialVariantView& variants, EditorTransactionManager& transactions, std::uint64_t generation) noexcept
	{
		const std::size_t variantCount = variants.Names.size();
		if (variantCount == 0)
		{
			UiUtil::DrawDetailsEmptyState();
			return;
		}

		if (!UiUtil::BeginDetailsCategory("Material Variants"))
		{
			return;
		}

		const MaterialVariantIndex activeVariantIndex = variants.Active;
		const char* preview = "Select Variant";
		std::string activeVariantName;
		if (activeVariantIndex < variantCount)
		{
			activeVariantName = variants.Names[activeVariantIndex];
			preview = activeVariantName.c_str();
		}

		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("##SceneMaterialVariant", preview))
		{
			for (std::size_t variantIndex = 0; variantIndex < variantCount; ++variantIndex)
			{
				const std::string& variantName = variants.Names[variantIndex];
				const bool selected = variantIndex == activeVariantIndex;
				if (ImGui::Selectable(variantName.c_str(), selected))
				{
					const MaterialVariantIndex selected = static_cast<MaterialVariantIndex>(variantIndex);
					if (activeVariantIndex != kInvalidMaterialVariantIndex)
						(void) transactions.Execute({0, SetMaterialVariantCommand{selected}},
						                           {0, SetMaterialVariantCommand{activeVariantIndex}}, generation);
					else
						(void) transactions.Execute({0, SetMaterialVariantCommand{selected}},
						                           {0, SetMaterialVariantCommand{selected}}, generation);
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
