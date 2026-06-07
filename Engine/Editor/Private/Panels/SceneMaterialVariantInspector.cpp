#include "PCH.h"

#include "Panels/SceneMaterialVariantInspector.h"

#include "Scene/GameScene.h"
#include "Scene/Materials/SceneMaterialVariants.h"
#include "Scene/Meshes/SceneMeshes.h"
#include "Util/UiUtil.h"

#include <imgui.h>

namespace SceneMaterialVariantInspector
{
	void Build(GameScene& gameScene) noexcept
	{
		SceneMaterialVariants& variants = gameScene.GetMaterialVariants();
		if (variants.GetVariantCount() == 0)
		{
			UiUtil::DrawDetailsEmptyState();
			return;
		}

		if (!UiUtil::BeginDetailsCategory("Material Variants"))
		{
			return;
		}

		const SceneMaterialVariantIndex activeVariantIndex = variants.GetActiveVariantIndex();
		const char* preview = "Select Variant";
		if (activeVariantIndex < variants.GetVariantCount())
		{
			preview = variants.GetVariant(activeVariantIndex).name.c_str();
		}

		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("##SceneMaterialVariant", preview))
		{
			for (std::size_t variantIndex = 0; variantIndex < variants.GetVariantCount(); ++variantIndex)
			{
				const SceneMaterialVariantDesc& variant = variants.GetVariant(variantIndex);
				const bool selected = variantIndex == activeVariantIndex;
				if (ImGui::Selectable(variant.name.c_str(), selected))
				{
					variants.ApplyVariant(static_cast<SceneMaterialVariantIndex>(variantIndex), gameScene.GetMeshes());
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
