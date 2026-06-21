#pragma once

#include "Frame/Core/FrameContext.h"
#include "RHI/Public/Bindings/RenderBindingSet.h"
#include "SceneData/MaterialTextureTableCapability.h"

#include <cstdint>

namespace MaterialTextureTablePassBinding
{
	template <typename TParameterInstance>
	bool Bind(TParameterInstance& parameters, const FrameContext& frame) noexcept
	{
		const RenderBindingSet* materialTextureTable = frame.sceneData.materialTextureTable;
		if (!frame.sceneData.materialTextureTableValid || materialTextureTable == nullptr || !*materialTextureTable)
		{
			return false;
		}

		const std::uint32_t descriptorCount = frame.sceneData.materialTextureTableDescriptorCount;
		if (descriptorCount == 0u || descriptorCount > MaterialTextureTableFixedCapacity || materialTextureTable->GetDescriptorCount() < descriptorCount)
		{
			return false;
		}

		parameters->MaterialTextureTable = materialTextureTable->GetTableBinding(0);
		return true;
	}
}
