#include "PCH.h"

#include "Gltf/GltfMaterialPropertyMapper.h"

#include <DirectXMath.h>
#include <cgltf.h>

#include <algorithm>

void GltfMaterialPropertyMapper::Apply(const cgltf_material& material, ImportedMaterial& importedMaterial)
{
	importedMaterial.emissiveColor =
	    DirectX::XMFLOAT3(material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]);
	if (material.has_emissive_strength)
	{
		importedMaterial.emissiveColor.x *= material.emissive_strength.emissive_strength;
		importedMaterial.emissiveColor.y *= material.emissive_strength.emissive_strength;
		importedMaterial.emissiveColor.z *= material.emissive_strength.emissive_strength;
	}

	importedMaterial.alphaCutoff = material.alpha_cutoff;
	importedMaterial.doubleSided = material.double_sided;

	switch (material.alpha_mode)
	{
		case cgltf_alpha_mode_mask:
			importedMaterial.alphaMode = ImportedAlphaMode::Mask;
			break;
		case cgltf_alpha_mode_blend:
			importedMaterial.alphaMode = ImportedAlphaMode::Blend;
			break;
		case cgltf_alpha_mode_opaque:
		default:
			importedMaterial.alphaMode = ImportedAlphaMode::Opaque;
			break;
	}

	if (material.has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness& pbr = material.pbr_metallic_roughness;
		importedMaterial.baseColor =
		    DirectX::XMFLOAT4(pbr.base_color_factor[0], pbr.base_color_factor[1], pbr.base_color_factor[2], pbr.base_color_factor[3]);
		importedMaterial.metallic = pbr.metallic_factor;
		importedMaterial.roughness = pbr.roughness_factor;
	}

	if (material.has_ior)
	{
		const float ior = (std::max) (material.ior.ior, 0.0f);
		const float f0 = ior > 0.0f ? (ior - 1.0f) / (ior + 1.0f) : 0.0f;
		importedMaterial.f0 = (std::clamp) (f0 * f0, 0.0f, 1.0f);
	}
}
